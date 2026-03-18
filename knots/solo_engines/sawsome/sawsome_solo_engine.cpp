#include "knots/solo_engines/sawsome/sawsome_solo_engine.h"

namespace {
constexpr int32_t kUnityQ12 = 4096;
// Detune offsets for the shared 7-voice spread. These match the integrated
// engine so standalone voicing and `mtws` slot voicing stay identical.
constexpr int32_t kDetuneOffsetQ16[SawsomeSoloEngine::kNumVoices] = {
    -3920, -2370, -910, 0, 1040, 2610, 3550,
};
// Stereo pan positions (-1..+1 in Q12) for each voice.
constexpr int32_t kPanQ12[SawsomeSoloEngine::kNumVoices] = {
    -3686, -2458, -1229, 0, 1229, 2458, 3686,
};
// Voice gain taper that emphasizes center voice while preserving side energy.
constexpr int32_t kGainQ12[SawsomeSoloEngine::kNumVoices] = {
    2048, 2867, 3482, 4096, 3482, 2867, 2048,
};
// Center voice index in the symmetric 7-voice spread.
constexpr uint8_t kCenterVoice = 3;
// Sum of squared full-spread voice gains. Used as the reference for makeup gain.
constexpr uint32_t kFullSpreadPowerQ24 = 65853850U;
// Makeup gain ceiling in Q12. 2.0x is enough to recover nearly constant power
// from center-only up to the 7-voice spread without letting gain run away.
constexpr uint32_t kMaxMakeupQ12 = 8192U;

// Converts 0..4095 control values into Q12 interpolation range 0..4096.
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }

// Integer square root for 64-bit inputs. Used at control rate to derive a
// bounded makeup gain without introducing float math into the engine path.
uint32_t IntegerSqrt64(uint64_t v) {
  uint64_t bit = 1ULL << 62;
  uint64_t result = 0;

  while (bit > v) {
    bit >>= 2;
  }

  while (bit != 0) {
    if (v >= result + bit) {
      v -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }

  return uint32_t(result);
}
}  // namespace

// Initializes dependency and per-voice runtime state.
SawsomeSoloEngine::SawsomeSoloEngine(mtws::SineLUT* lut) : lut_(lut) {
  Init();
}

// Resets phases (with decorrelation seeds) and triangle integrator state.
void SawsomeSoloEngine::Init() {
  (void)lut_;
  for (uint8_t i = 0; i < kNumVoices; ++i) {
    phases_[i] = uint32_t(i) * 0x1f123bb5U;
    tri_state_[i] = 0;
  }
}

// Converts controls into per-voice detune increments and stereo panning gains.
void SawsomeSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  uint32_t width_q12 = ToQ12(control.macro_x);
  uint32_t detune_q12 = ToQ12(control.macro_y);
  uint32_t active_gain_q12[kNumVoices];
  out.alt = control.alt;

  // Fade side voices in with detune so full CCW collapses to the center voice,
  // while full CW restores the full 7-voice gain distribution.
  uint32_t active_power_q24 = 0;
  for (uint8_t i = 0; i < kNumVoices; ++i) {
    uint32_t voice_gain_q12 = uint32_t(kGainQ12[i]);
    if (i != kCenterVoice) {
      voice_gain_q12 = uint32_t((uint64_t(voice_gain_q12) * detune_q12 + 2048U) >> 12);
    }
    active_gain_q12[i] = voice_gain_q12;
    active_power_q24 += voice_gain_q12 * voice_gain_q12;
  }

  // Approximate constant-energy compensation: measure the active voice power,
  // then apply a bounded amplitude makeup factor derived from the full-spread
  // reference power. This keeps Y movement from sounding like a large volume
  // jump while preserving headroom.
  uint32_t makeup_q12 = 4096U;
  if (active_power_q24 > 0U) {
    uint64_t makeup_squared_q24 = (uint64_t(kFullSpreadPowerQ24) << 24) / active_power_q24;
    makeup_q12 = IntegerSqrt64(makeup_squared_q24);
    if (makeup_q12 > kMaxMakeupQ12) makeup_q12 = kMaxMakeupQ12;
  }

  for (uint8_t i = 0; i < kNumVoices; ++i) {
    // Ratio = 1.0 + (voice detune offset * detune amount).
    int32_t ratio_q16 = 65536 + int32_t((int64_t(kDetuneOffsetQ16[i]) * int32_t(detune_q12)) >> 12);
    if (ratio_q16 < 32768) ratio_q16 = 32768;

    uint64_t inc = (uint64_t(control.pitch_inc) * uint32_t(ratio_q16)) >> 16;
    if (inc > 0x7FFFFFFFULL) inc = 0x7FFFFFFFULL;
    out.phase_increment[i] = uint32_t(inc);
    out.phase_inc_recip_q24[i] = (inc > 0U) ? uint32_t((1ULL << 36) / inc) : 0U;

    // Pan law: width scales fixed voice pan positions, then converts them to
    // left/right gains. Keeping the exact shared pan table means the solo
    // engine and the integrated engine keep the same stereo image.
    int32_t pan_q12 = int32_t((int64_t(kPanQ12[i]) * int32_t(width_q12)) >> 12);
    int32_t voice_gain_q12 = int32_t((uint64_t(active_gain_q12[i]) * makeup_q12 + 2048U) >> 12);
    int32_t gain_l = int32_t((int64_t(voice_gain_q12) * (kUnityQ12 - pan_q12)) >> 13);
    int32_t gain_r = int32_t((int64_t(voice_gain_q12) * (kUnityQ12 + pan_q12)) >> 13);
    if (gain_l < 0) gain_l = 0;
    if (gain_r < 0) gain_r = 0;
    out.gain_l_q12[i] = int16_t(gain_l);
    out.gain_r_q12[i] = int16_t(gain_r);
  }
}

// Clamps mixed sample to signed 12-bit board output range.
int32_t SawsomeSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Generates a saw and applies PolyBLEP correction at discontinuity regions.
// The precomputed reciprocal keeps the standalone audio path in sync with the
// integrated engine's lower-cost multiply-based BLEP math.
int32_t SawsomeSoloEngine::PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc, uint32_t recip_q24) {
  int32_t saw = int32_t((phase >> 20) & 0x0FFFU) - 2048;
  if (phase_inc == 0U) return saw;

  if (phase < phase_inc) {
    // Leading-edge correction.
    uint32_t t_q12 = uint32_t((uint64_t(phase) * recip_q24) >> 24);
    int32_t x = int32_t(t_q12);
    int32_t corr = x + x - int32_t((int64_t(x) * x) >> 12) - 4096;
    saw -= corr >> 1;
  }

  uint32_t tail = 0xFFFFFFFFU - phase;
  if (tail < phase_inc) {
    // Trailing-edge correction.
    uint32_t t_q12 = uint32_t((uint64_t(tail) * recip_q24) >> 24);
    int32_t x = int32_t(t_q12);
    int32_t corr = int32_t((int64_t(x) * x) >> 12);
    saw += corr >> 1;
  }

  return Clamp12(saw);
}

// Triangle is synthesized by leaky integration of the band-limited saw.
int32_t SawsomeSoloEngine::PolyBlepTriangleQ12(int voice_index, uint32_t phase, uint32_t phase_inc, uint32_t recip_q24) {
  int32_t saw = PolyBlepSawQ12(phase, phase_inc, recip_q24);
  tri_state_[voice_index] += saw;
  tri_state_[voice_index] -= tri_state_[voice_index] >> 11;
  return Clamp12(tri_state_[voice_index] >> 5);
}

// Renders and sums all seven voices, then applies final output clamp.
void SawsomeSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  int32_t sum_l = 0;
  int32_t sum_r = 0;

  for (uint8_t i = 0; i < kNumVoices; ++i) {
    phases_[i] += frame.phase_increment[i];
    int32_t s = frame.alt ? PolyBlepTriangleQ12(i, phases_[i], frame.phase_increment[i], frame.phase_inc_recip_q24[i])
                          : PolyBlepSawQ12(phases_[i], frame.phase_increment[i], frame.phase_inc_recip_q24[i]);
    sum_l += int32_t((int64_t(s) * frame.gain_l_q12[i]) >> 12);
    sum_r += int32_t((int64_t(s) * frame.gain_r_q12[i]) >> 12);
  }

  out1 = Clamp12(sum_l >> 1);
  out2 = Clamp12(sum_r >> 1);
}
