#include "prex/mtws/engines/additive_engine.h"

namespace mtws {

AdditiveEngine::AdditiveEngine(SineLUT* lut) : lut_(lut) {
  Init();
}

void AdditiveEngine::Init() {
  for (int i = 0; i < kNumAdditiveVoices; ++i) {
    phases_[i] = 0;
    smoothed_ratio_q16_[i] = HarmonicRatioQ16(i);
    smoothed_gain_q12_[i] = int32_t(kUnityQ12 / uint32_t(i + 1));
  }
}

void AdditiveEngine::OnSelected() {
  // Keep phase continuity to avoid clicks when re-entering this engine.
}

int32_t AdditiveEngine::CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12) {
  uint32_t pos_q12 = uint32_t(partial_index) << 12;
  uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
  uint32_t denom_q12 = kUnityQ12 + d_q12;
  uint64_t numer = uint64_t(kUnityQ12) * kUnityQ12;
  return int32_t((numer + (denom_q12 >> 1)) / denom_q12);
}

int32_t AdditiveEngine::CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12) {
  uint32_t pos_q12 = uint32_t(partial_index) << 12;
  uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
  if (d_q12 >= kUnityQ12) return 0;
  return int32_t(kUnityQ12 - d_q12);
}

int32_t AdditiveEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > kUnityQ12) t_q12 = kUnityQ12;
  return int32_t((int64_t(a) * int32_t(kUnityQ12 - t_q12) + int64_t(b) * int32_t(t_q12)) >> 12);
}

int32_t AdditiveEngine::SmoothToward(int32_t current, int32_t target, int shift) {
  if (shift <= 0) return target;
  int32_t delta = target - current;
  int32_t step = delta >> shift;
  if (step == 0 && delta != 0) {
    step = (delta > 0) ? 1 : -1;
  }
  return current + step;
}

int32_t AdditiveEngine::FoldReflectQ16(int32_t x, int32_t lo, int32_t hi) {
  int64_t span = int64_t(hi) - int64_t(lo);
  if (span <= 0) return lo;
  int64_t period = span << 1;
  int64_t t = (int64_t(x) - int64_t(lo)) % period;
  if (t < 0) t += period;
  if (t > span) t = period - t;
  return int32_t(int64_t(lo) + t);
}

int32_t AdditiveEngine::ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y) {
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

void AdditiveEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  AdditiveControlFrame& out = frame.additive;

  uint32_t knob_x = global.macro_x;
  uint32_t knob_y = global.macro_y;
  uint32_t base_inc = global.pitch_inc;
  bool alt_mode = global.mode_alt;

  if (knob_x > 4095U) knob_x = 4095U;
  uint32_t centroid_max_q12 = uint32_t(kNumAdditiveVoices - 1) << 12;
  uint32_t centroid_q12 = uint32_t((uint64_t(knob_x) * centroid_max_q12 + 2048U) >> 12);
  int32_t centroid_ratio_q16 = int32_t((1U << 16) + (centroid_q12 << 4));

  int32_t max_ratio_q16 = kMaxRatioQ16;
  if (base_inc > 0U) {
    uint32_t max_ratio_from_nyquist_q16 = uint32_t((uint64_t(kNyquistPhaseInc) << 16) / base_inc);
    if (max_ratio_from_nyquist_q16 < uint32_t(max_ratio_q16)) {
      max_ratio_q16 = int32_t(max_ratio_from_nyquist_q16);
    }
  }
  if (max_ratio_q16 < kMinRatioQ16) max_ratio_q16 = kMinRatioQ16;

  int32_t gain_sum_q12 = 0;
  for (int i = 0; i < kNumAdditiveVoices; ++i) {
    int32_t target_ratio_q16 = HarmonicRatioQ16(i);

    if (alt_mode) {
      if (i == 0 && knob_y > 2048U) {
        target_ratio_q16 = HarmonicRatioQ16(0);
      } else {
        if (knob_y < 2048U) {
          uint32_t warp_q12 = (2048U - knob_y) << 1;
          target_ratio_q16 = LerpQ12(target_ratio_q16, centroid_ratio_q16, warp_q12);
        } else if (knob_y > 2048U) {
          uint32_t warp_q12 = (knob_y - 2048U) << 1;
          if (knob_y >= 4095U) warp_q12 = kUnityQ12;
          int32_t repel_ratio_q16 = (target_ratio_q16 << 1) - centroid_ratio_q16;
          target_ratio_q16 = LerpQ12(target_ratio_q16, repel_ratio_q16, warp_q12);
        }

        target_ratio_q16 = FoldReflectQ16(target_ratio_q16, HarmonicRatioQ16(0), max_ratio_q16);
      }

      if (target_ratio_q16 < kMinRatioQ16) target_ratio_q16 = kMinRatioQ16;
      if (target_ratio_q16 > max_ratio_q16) target_ratio_q16 = max_ratio_q16;
    }

    smoothed_ratio_q16_[i] = SmoothToward(smoothed_ratio_q16_[i], target_ratio_q16, kRatioSmoothShift);
    if (smoothed_ratio_q16_[i] < kMinRatioQ16) smoothed_ratio_q16_[i] = kMinRatioQ16;
    if (smoothed_ratio_q16_[i] > max_ratio_q16) smoothed_ratio_q16_[i] = max_ratio_q16;

    uint32_t voice_inc = uint32_t((uint64_t(base_inc) * uint32_t(smoothed_ratio_q16_[i])) >> 16);
    if (voice_inc > kNyquistPhaseInc) voice_inc = kNyquistPhaseInc;
    out.voice_phase_increment[i] = voice_inc;

    int32_t target_gain_q12 = ShapedCentroidGainQ12(i, centroid_q12, knob_y);
    if (target_gain_q12 < 0) target_gain_q12 = 0;
    if (target_gain_q12 > int32_t(kUnityQ12)) target_gain_q12 = int32_t(kUnityQ12);

    smoothed_gain_q12_[i] = SmoothToward(smoothed_gain_q12_[i], target_gain_q12, kGainSmoothShift);
    if (smoothed_gain_q12_[i] < 0) smoothed_gain_q12_[i] = 0;
    if (smoothed_gain_q12_[i] > int32_t(kUnityQ12)) smoothed_gain_q12_[i] = int32_t(kUnityQ12);

    out.voice_gain_q12[i] = smoothed_gain_q12_[i];
    gain_sum_q12 += smoothed_gain_q12_[i];
  }

  if (gain_sum_q12 > 0) {
    uint64_t numer = uint64_t(kUnityQ12) * kUnityQ12;
    int32_t mix_peak_q12 = int32_t((numer + (uint32_t(gain_sum_q12) >> 1)) / uint32_t(gain_sum_q12));

    uint32_t makeup_q12 = kUnityQ12;
    if (knob_y > 2048U) {
      uint32_t t_q12 = (knob_y - 2048U) << 1;
      if (knob_y >= 4095U) t_q12 = kUnityQ12;
      makeup_q12 = kUnityQ12 + uint32_t((uint64_t(kMakeupMaxQ12 - kUnityQ12) * t_q12) >> 12);
    }

    out.mix_norm_q12 = int32_t((int64_t(mix_peak_q12) * makeup_q12) >> 12);
  } else {
    out.mix_norm_q12 = int32_t(kUnityQ12);
  }

  if (out.mix_norm_q12 < 0) out.mix_norm_q12 = 0;
  if (out.mix_norm_q12 > int32_t(kMakeupMaxQ12)) out.mix_norm_q12 = int32_t(kMakeupMaxQ12);
}

void AdditiveEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const AdditiveControlFrame& in = frame.additive;

  int32_t sum = 0;
  for (int i = 0; i < kNumAdditiveVoices; ++i) {
    phases_[i] += in.voice_phase_increment[i];
    int32_t s = lut_->LookupLinear(phases_[i]);
    sum += (s * in.voice_gain_q12[i]) >> 12;
  }

  int32_t out = int32_t((int64_t(sum) * in.mix_norm_q12) >> 12);
  if (out > 2047) out = 2047;
  if (out < -2048) out = -2048;

  out1 = out;
  out2 = out;
}

}  // namespace mtws
