#include "prex/cumulus/cumulus_solo_engine.h"

namespace {
// Converts 0..4095 knob range into 0..4096 Q12 interpolation range.
// 4096 allows exact "1.0" interpolation at full clockwise.
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }
}  // namespace

// Initializes dependency and runtime state arrays.
CumulusSoloEngine::CumulusSoloEngine(mtws::SineLUT* lut) : lut_(lut) {
  Init();
}

// Resets partial phases and smoothing memory to harmonic defaults.
void CumulusSoloEngine::Init() {
  for (int i = 0; i < 16; ++i) {
    phases_[i] = 0;
    smoothed_ratio_q16_[i] = HarmonicRatioQ16(i);
    smoothed_gain_q12_[i] = int32_t(kUnityQ12 / uint32_t(i + 1));
  }
}

// Clamps output to signed 12-bit board domain.
int32_t CumulusSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Interpolates between a and b using a Q12 fraction.
int32_t CumulusSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > kUnityQ12) t_q12 = kUnityQ12;
  return int32_t((int64_t(a) * int32_t(kUnityQ12 - t_q12) + int64_t(b) * int32_t(t_q12)) >> 12);
}

// One-pole integer smoother used for zipper-noise suppression on controls.
int32_t CumulusSoloEngine::SmoothToward(int32_t current, int32_t target, int shift) {
  if (shift <= 0) return target;
  int32_t delta = target - current;
  int32_t step = delta >> shift;
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return current + step;
}

// Reflect-folds x into [lo, hi] by mirroring at boundaries.
// This keeps warped ratios bounded without hard clipping discontinuities.
int32_t CumulusSoloEngine::FoldReflectQ16(int32_t x, int32_t lo, int32_t hi) {
  int64_t span = int64_t(hi) - int64_t(lo);
  if (span <= 0) return lo;
  int64_t period = span << 1;
  int64_t t = (int64_t(x) - int64_t(lo)) % period;
  if (t < 0) t += period;
  if (t > span) t = period - t;
  return int32_t(int64_t(lo) + t);
}

// Broad centroid shape: gain falls as 1/(1+distance) in partial-space Q12.
int32_t CumulusSoloEngine::CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12) {
  uint32_t pos_q12 = uint32_t(partial_index) << 12;
  uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
  uint32_t denom_q12 = kUnityQ12 + d_q12;
  uint64_t numer = uint64_t(kUnityQ12) * kUnityQ12;
  return int32_t((numer + (denom_q12 >> 1)) / denom_q12);
}

// Narrow centroid shape: linear drop to zero outside one partial.
int32_t CumulusSoloEngine::CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12) {
  uint32_t pos_q12 = uint32_t(partial_index) << 12;
  uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
  if (d_q12 >= kUnityQ12) return 0;
  return int32_t(kUnityQ12 - d_q12);
}

// Macro Y shape morph:
// - lower half: narrow -> broad centroid
// - upper half: broad centroid -> flat (all partials equal)
int32_t CumulusSoloEngine::ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y) {
  if (knob_y > 4095U) knob_y = 4095U;
  int32_t g_base = CentroidBaseGainQ12(partial_index, centroid_q12);
  int32_t g_narrow = CentroidNarrowGainQ12(partial_index, centroid_q12);

  if (knob_y <= 2048U) {
    uint32_t t_q12 = knob_y << 1;
    return LerpQ12(g_narrow, g_base, t_q12);
  }

  uint32_t t_q12 = (knob_y - 2048U) << 1;
  if (knob_y >= 4095U) t_q12 = kUnityQ12;
  return LerpQ12(g_base, int32_t(kUnityQ12), t_q12);
}

// Precomputes all per-partial frequencies and gains for one control frame.
void CumulusSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  uint32_t knob_x = control.macro_x;
  uint32_t knob_y = control.macro_y;
  uint32_t base_inc = control.pitch_inc;
  bool alt_mode = control.alt;

  if (knob_x > 4095U) knob_x = 4095U;
  // Map X across partial indices [0..15] in Q12 so centroid can sit between bins.
  uint32_t centroid_max_q12 = uint32_t(16 - 1) << 12;
  uint32_t centroid_q12 = uint32_t((uint64_t(knob_x) * centroid_max_q12 + 2048U) >> 12);
  // Convert centroid index into a ratio target domain (Q16).
  int32_t centroid_ratio_q16 = int32_t((1U << 16) + (centroid_q12 << 4));

  int32_t max_ratio_q16 = kMaxRatioQ16;

  uint32_t odd_gain_sum_q12 = 0;
  uint32_t even_gain_sum_q12 = 0;
  for (int i = 0; i < 16; ++i) {
    // Default is harmonic stack: 1f, 2f, 3f, ... in Q16.
    int32_t target_ratio_q16 = HarmonicRatioQ16(i);

    if (alt_mode) {
      if (i == 0 && knob_y > 2048U) {
        // Keep fundamental anchored when upper-half warp would repel harmonics.
        target_ratio_q16 = HarmonicRatioQ16(0);
      } else {
        if (knob_y < 2048U) {
          // Lower-half Y attracts ratios toward centroid ratio.
          uint32_t warp_q12 = (2048U - knob_y) << 1;
          target_ratio_q16 = LerpQ12(target_ratio_q16, centroid_ratio_q16, warp_q12);
        } else if (knob_y > 2048U) {
          // Upper-half Y repels ratios away from centroid mirror point.
          uint32_t warp_q12 = (knob_y - 2048U) << 1;
          if (knob_y >= 4095U) warp_q12 = kUnityQ12;
          int32_t repel_ratio_q16 = (target_ratio_q16 << 1) - centroid_ratio_q16;
          target_ratio_q16 = LerpQ12(target_ratio_q16, repel_ratio_q16, warp_q12);
        }

        // Reflection keeps warped ratios bounded while preserving motion continuity.
        target_ratio_q16 = FoldReflectQ16(target_ratio_q16, HarmonicRatioQ16(0), max_ratio_q16);
      }

      if (target_ratio_q16 < kMinRatioQ16) target_ratio_q16 = kMinRatioQ16;
      if (target_ratio_q16 > max_ratio_q16) target_ratio_q16 = max_ratio_q16;
    }

    if (alt_mode) {
      smoothed_ratio_q16_[i] = SmoothToward(smoothed_ratio_q16_[i], target_ratio_q16, kRatioSmoothShift);
      if (smoothed_ratio_q16_[i] < kMinRatioQ16) smoothed_ratio_q16_[i] = kMinRatioQ16;
      if (smoothed_ratio_q16_[i] > max_ratio_q16) smoothed_ratio_q16_[i] = max_ratio_q16;
    } else {
      smoothed_ratio_q16_[i] = target_ratio_q16;
    }

    uint64_t voice_inc_64 = (uint64_t(base_inc) * uint64_t(uint32_t(smoothed_ratio_q16_[i]))) >> 16;
    bool voice_above_guard = (voice_inc_64 > uint64_t(kPartialMaxPhaseInc));
    if (voice_inc_64 > uint64_t(kNyquistPhaseInc)) voice_inc_64 = uint64_t(kNyquistPhaseInc);
    out.voice_phase_increment[i] = uint32_t(voice_inc_64);

    if (voice_above_guard) {
      // Extra anti-alias protection: mute partials nearing unsafe increment zone.
      smoothed_gain_q12_[i] = 0;
      out.voice_gain_q12[i] = 0;
    } else {
      int32_t target_gain_q12 = ShapedCentroidGainQ12(i, centroid_q12, knob_y);
      if (target_gain_q12 < 0) target_gain_q12 = 0;
      if (target_gain_q12 > int32_t(kUnityQ12)) target_gain_q12 = int32_t(kUnityQ12);

      smoothed_gain_q12_[i] = SmoothToward(smoothed_gain_q12_[i], target_gain_q12, kGainSmoothShift);
      if (smoothed_gain_q12_[i] < 0) smoothed_gain_q12_[i] = 0;
      if (smoothed_gain_q12_[i] > int32_t(kUnityQ12)) smoothed_gain_q12_[i] = int32_t(kUnityQ12);

      // Uniform partial pre-gain keeps perceived level stable across centroid shapes.
      int32_t scaled_gain_q12 = int32_t((int64_t(smoothed_gain_q12_[i]) * int32_t(kUniformPartialGainQ12) + 2048) >> 12);
      if (scaled_gain_q12 < 0) scaled_gain_q12 = 0;
      out.voice_gain_q12[i] = scaled_gain_q12;
    }

    uint32_t g_q12 = uint32_t(out.voice_gain_q12[i]);
    if ((i & 1) == 0) {
      odd_gain_sum_q12 += g_q12;
    } else {
      even_gain_sum_q12 += g_q12;
    }
  }

  uint32_t max_bus_gain_sum_q12 = (odd_gain_sum_q12 > even_gain_sum_q12) ? odd_gain_sum_q12 : even_gain_sum_q12;
  if (max_bus_gain_sum_q12 == 0U) {
    out.mix_norm_q12 = int32_t(kCumulusMasterGainQ12);
    return;
  }

  // Estimate worst-case pre-mix peak on the louder stereo bus.
  uint32_t bus_peak_pre_mix = uint32_t((uint64_t(2047U) * max_bus_gain_sum_q12) >> 12);
  bus_peak_pre_mix >>= kRenderSumShift;
  if (bus_peak_pre_mix == 0U) bus_peak_pre_mix = 1U;

  // Compute Q12 gain that normalizes estimated peak to full-scale, then cap it.
  uint32_t safe_mix_norm_q12 = uint32_t(((uint64_t(2047U) << 12) + (bus_peak_pre_mix >> 1)) / uint64_t(bus_peak_pre_mix));
  if (safe_mix_norm_q12 > kCumulusMasterGainQ12) safe_mix_norm_q12 = kCumulusMasterGainQ12;
  out.mix_norm_q12 = int32_t(safe_mix_norm_q12);
}

// Renders additive stereo sample:
// - even-indexed partials feed out1
// - odd-indexed partials feed out2
// Then applies fixed headroom shift and frame-level normalization.
void CumulusSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  int32_t sum_odd = 0;
  int32_t sum_even = 0;

  for (int i = 0; i < 16; ++i) {
    phases_[i] += frame.voice_phase_increment[i];
    int32_t s = lut_->LookupLinear(phases_[i]);
    int32_t voice = int32_t((int64_t(s) * frame.voice_gain_q12[i]) >> 12);

    if ((i & 1) == 0) {
      sum_odd += voice;
    } else {
      sum_even += voice;
    }
  }

  int32_t odd_bus = sum_odd >> kRenderSumShift;
  int32_t even_bus = sum_even >> kRenderSumShift;

  out1 = Clamp12(int32_t((int64_t(odd_bus) * frame.mix_norm_q12) >> 12));
  out2 = Clamp12(int32_t((int64_t(even_bus) * frame.mix_norm_q12) >> 12));
}
