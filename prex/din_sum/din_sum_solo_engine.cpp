#include "prex/din_sum/din_sum_solo_engine.h"

namespace {
// Converts knob range 0..4095 to Q12 interpolation range 0..4096.
inline uint16_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : v; }
}  // namespace

// Initializes dependency and internal RNG/filter state.
DinSumSoloEngine::DinSumSoloEngine(mtws::SineLUT* lut)
    : lut_(lut), noise_state_(1U), pink_state_(0), bp1_state_{0, 0}, bp2_state_{0, 0}, hp_state_{0, 0}, lp_state_{0, 0} {
  Init();
}

// Resets PRNG and all filter integrators to deterministic startup values.
void DinSumSoloEngine::Init() {
  (void)lut_;
  noise_state_ = 1U;
  pink_state_ = 0;
  bp1_state_ = {0, 0};
  bp2_state_ = {0, 0};
  hp_state_ = {0, 0};
  lp_state_ = {0, 0};
}

// Clamps output to signed 12-bit board audio domain.
int32_t DinSumSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Converts cutoff in Hz to an SVF coefficient (`f ~= cutoff / (sample_rate / 2)`).
// Clamps to a stable range so the fixed-point SVF remains well behaved.
uint16_t DinSumSoloEngine::HzToFQ15(uint32_t hz) {
  if (hz < 20U) hz = 20U;
  if (hz > 10000U) hz = 10000U;
  uint32_t f_q15 = (hz * 32768U + 12000U) / 24000U;
  if (f_q15 > 32767U) f_q15 = 32767U;
  if (f_q15 < 32U) f_q15 = 32U;
  return uint16_t(f_q15);
}

// Linear interpolation where t_u12 is interpreted as Q12 in [0, 4096].
uint32_t DinSumSoloEngine::LerpU32(uint32_t a, uint32_t b, uint16_t t_u12) {
  uint32_t t_q12 = (t_u12 >= 4095U) ? 4096U : uint32_t(t_u12);
  return uint32_t(int64_t(a) + ((int64_t(b) - int64_t(a)) * int64_t(t_q12) >> 12));
}

// Performs one Chamberlin-style SVF update and returns high-pass output.
int32_t DinSumSoloEngine::SVFHighpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return hp;
}

// Performs one Chamberlin-style SVF update and returns low-pass output.
int32_t DinSumSoloEngine::SVFLowpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return state.lp;
}

// Performs one Chamberlin-style SVF update and returns band-pass output.
int32_t DinSumSoloEngine::SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return state.bp;
}

// Converts controls into filter frequencies and resonance for one render frame.
void DinSumSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  // Convert phase increment to approximate Hz at 48 kHz sample rate.
  uint32_t pitch_hz = uint32_t((uint64_t(control.pitch_inc) * 48000U) >> 32);
  if (pitch_hz < 20U) pitch_hz = 20U;

  uint16_t y_q12 = ToQ12(control.macro_y);
  uint16_t x_q12 = ToQ12(control.macro_x);

  // Y spreads around the pitch anchor: low branch shifts downward, high upward.
  uint32_t low_hz = LerpU32(pitch_hz, pitch_hz >> 1, y_q12);
  uint32_t high_hz = LerpU32(pitch_hz, pitch_hz << 1, y_q12);

  if (low_hz < 20U) low_hz = 20U;
  if (high_hz > 10000U) high_hz = 10000U;

  out.low_f_q15 = HzToFQ15(low_hz);
  out.high_f_q15 = HzToFQ15(high_hz);
  out.damping_q12 = uint16_t(3400U - ((uint32_t(x_q12) * 2600U) >> 12));
  out.alt = control.alt;
}

// Renders one sample:
// - Generate white noise from xorshift32.
// - Optionally smooth to pink-ish source in alt mode.
// - Route through either two BPs or HP/LP split.
void DinSumSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  // xorshift32 PRNG: cheap and deterministic for audio-rate noise.
  noise_state_ ^= noise_state_ << 13;
  noise_state_ ^= noise_state_ >> 17;
  noise_state_ ^= noise_state_ << 5;
  int32_t white = int32_t((noise_state_ >> 20) & 0x0FFFU) - 2048;

  int32_t src = white;
  if (frame.alt) {
    // One-pole averaging softens high-frequency noise energy.
    pink_state_ += (white - pink_state_) >> 4;
    src = pink_state_;
  }

  if (!frame.alt) {
    out1 = Clamp12(SVFBandpass(src, frame.low_f_q15, frame.damping_q12, bp1_state_));
    out2 = Clamp12(SVFBandpass(src, frame.high_f_q15, frame.damping_q12, bp2_state_));
    return;
  }

  out1 = Clamp12(SVFHighpass(src, frame.high_f_q15, frame.damping_q12, hp_state_));
  out2 = Clamp12(SVFLowpass(src, frame.low_f_q15, frame.damping_q12, lp_state_));
}
