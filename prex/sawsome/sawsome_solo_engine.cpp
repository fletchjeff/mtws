#include "prex/sawsome/sawsome_solo_engine.h"

namespace {
// Symmetric per-voice detune offsets around center voice in Q16 ratio space.
constexpr int32_t kDetuneOffsetQ16[7] = {-3932, -2621, -1311, 0, 1311, 2621, 3932};
// Stereo pan positions (-1..+1 in Q12) for each voice.
constexpr int32_t kPanQ12[7] = {-3686, -2458, -1229, 0, 1229, 2458, 3686};
// Voice gain taper that emphasizes center voice while preserving side energy.
constexpr int32_t kGainQ12[7] = {2048, 2867, 3482, 4096, 3482, 2867, 2048};

// Converts 0..4095 control values into Q12 interpolation range 0..4096.
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }
}  // namespace

// Initializes dependency and per-voice runtime state.
SawsomeSoloEngine::SawsomeSoloEngine(mtws::SineLUT* lut) : lut_(lut) {
  Init();
}

// Resets phases (with decorrelation seeds) and triangle integrator state.
void SawsomeSoloEngine::Init() {
  (void)lut_;
  for (uint8_t i = 0; i < 7; ++i) {
    phases_[i] = uint32_t(i) * 0x1f123bb5U;
    tri_state_[i] = 0;
  }
}

// Converts controls into per-voice detune increments and stereo panning gains.
void SawsomeSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  uint32_t width_q12 = ToQ12(control.macro_x);
  uint32_t detune_q12 = ToQ12(control.macro_y);
  out.alt = control.alt;

  for (uint8_t i = 0; i < 7; ++i) {
    // Ratio = 1.0 + (voice detune offset * detune amount).
    int32_t ratio_q16 = 65536 + int32_t((int64_t(kDetuneOffsetQ16[i]) * int32_t(detune_q12)) >> 12);
    if (ratio_q16 < 32768) ratio_q16 = 32768;

    uint64_t inc = (uint64_t(control.pitch_inc) * uint32_t(ratio_q16)) >> 16;
    if (inc > 0x7FFFFFFFULL) inc = 0x7FFFFFFFULL;
    out.phase_increment[i] = uint32_t(inc);

    // Pan law: width scales fixed voice pan positions, then convert to L/R gains.
    int32_t pan_q12 = int32_t((int64_t(kPanQ12[i]) * int32_t(width_q12)) >> 12);
    int32_t gain_l = int32_t((int64_t(kGainQ12[i]) * (4096 - pan_q12)) >> 13);
    int32_t gain_r = int32_t((int64_t(kGainQ12[i]) * (4096 + pan_q12)) >> 13);
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
// `phase_inc` controls correction width so higher pitches smooth wider in phase.
int32_t SawsomeSoloEngine::PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc) {
  int32_t saw = int32_t((phase >> 20) & 0x0FFFU) - 2048;
  if (phase_inc == 0U) return saw;

  if (phase < phase_inc) {
    // Leading-edge correction.
    uint32_t t_q12 = uint32_t((uint64_t(phase) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = x + x - int32_t((int64_t(x) * x) >> 12) - 4096;
    saw -= corr >> 1;
  }

  uint32_t tail = 0xFFFFFFFFU - phase;
  if (tail < phase_inc) {
    // Trailing-edge correction.
    uint32_t t_q12 = uint32_t((uint64_t(tail) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = int32_t((int64_t(x) * x) >> 12);
    saw += corr >> 1;
  }

  return Clamp12(saw);
}

// Triangle is synthesized by leaky integration of the band-limited saw.
int32_t SawsomeSoloEngine::PolyBlepTriangleQ12(int voice_index, uint32_t phase, uint32_t phase_inc) {
  int32_t saw = PolyBlepSawQ12(phase, phase_inc);
  tri_state_[voice_index] += saw;
  tri_state_[voice_index] -= tri_state_[voice_index] >> 11;
  return Clamp12(tri_state_[voice_index] >> 5);
}

// Renders and sums all seven voices, then applies final output clamp.
void SawsomeSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  int32_t sum_l = 0;
  int32_t sum_r = 0;

  for (uint8_t i = 0; i < 7; ++i) {
    phases_[i] += frame.phase_increment[i];
    int32_t s = frame.alt ? PolyBlepTriangleQ12(i, phases_[i], frame.phase_increment[i])
                          : PolyBlepSawQ12(phases_[i], frame.phase_increment[i]);
    sum_l += int32_t((int64_t(s) * frame.gain_l_q12[i]) >> 12);
    sum_r += int32_t((int64_t(s) * frame.gain_r_q12[i]) >> 12);
  }

  out1 = Clamp12(sum_l >> 1);
  out2 = Clamp12(sum_r >> 1);
}
