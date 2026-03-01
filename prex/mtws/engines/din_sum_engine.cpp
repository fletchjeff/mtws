#include "prex/mtws/engines/din_sum_engine.h"

namespace mtws {

namespace {
inline uint16_t ToQ12(uint16_t v) {
  return (v >= 4095U) ? 4096U : v;
}
}  // namespace

DinSumEngine::DinSumEngine()
    : noise_state_(1U), pink_state_(0), bp1_state_{0, 0}, bp2_state_{0, 0}, hp_state_{0, 0}, lp_state_{0, 0} {}

void DinSumEngine::Init() {
  noise_state_ = 1U;
  pink_state_ = 0;
  bp1_state_ = {0, 0};
  bp2_state_ = {0, 0};
  hp_state_ = {0, 0};
  lp_state_ = {0, 0};
}

void DinSumEngine::OnSelected() {
  // Keep filter states for continuity.
}

int32_t DinSumEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

uint16_t DinSumEngine::HzToFQ15(uint32_t hz) {
  if (hz < 20U) hz = 20U;
  if (hz > 10000U) hz = 10000U;
  uint32_t f_q15 = (hz * 32768U + 12000U) / 24000U;
  if (f_q15 > 32767U) f_q15 = 32767U;
  if (f_q15 < 32U) f_q15 = 32U;
  return uint16_t(f_q15);
}

uint32_t DinSumEngine::LerpU32(uint32_t a, uint32_t b, uint16_t t_u12) {
  uint32_t t_q12 = (t_u12 >= 4095U) ? 4096U : uint32_t(t_u12);
  return uint32_t(int64_t(a) + ((int64_t(b) - int64_t(a)) * int64_t(t_q12) >> 12));
}

int32_t DinSumEngine::SVFHighpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return hp;
}

int32_t DinSumEngine::SVFLowpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return state.lp;
}

int32_t DinSumEngine::SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return state.bp;
}

void DinSumEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  DinSumControlFrame& out = frame.din_sum;

  uint32_t pitch_hz = uint32_t((uint64_t(global.pitch_inc) * 48000U) >> 32);
  if (pitch_hz < 20U) pitch_hz = 20U;

  uint16_t y_q12 = ToQ12(global.macro_y);
  uint16_t x_q12 = ToQ12(global.macro_x);

  uint32_t low_hz = LerpU32(pitch_hz, pitch_hz >> 1, y_q12);
  uint32_t high_hz = LerpU32(pitch_hz, pitch_hz << 1, y_q12);

  if (low_hz < 20U) low_hz = 20U;
  if (high_hz > 10000U) high_hz = 10000U;

  out.low_f_q15 = HzToFQ15(low_hz);
  out.high_f_q15 = HzToFQ15(high_hz);
  out.damping_q12 = uint16_t(3400U - ((uint32_t(x_q12) * 2600U) >> 12));
  out.alt = global.mode_alt;
}

void DinSumEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const DinSumControlFrame& in = frame.din_sum;

  noise_state_ ^= noise_state_ << 13;
  noise_state_ ^= noise_state_ >> 17;
  noise_state_ ^= noise_state_ << 5;
  int32_t white = int32_t((noise_state_ >> 20) & 0x0FFFU) - 2048;

  int32_t src = white;
  if (in.alt) {
    // Pink-ish source by low-passing white noise.
    pink_state_ += (white - pink_state_) >> 4;
    src = pink_state_;
  }

  if (!in.alt) {
    out1 = Clamp12(SVFBandpass(src, in.low_f_q15, in.damping_q12, bp1_state_));
    out2 = Clamp12(SVFBandpass(src, in.high_f_q15, in.damping_q12, bp2_state_));
    return;
  }

  out1 = Clamp12(SVFHighpass(src, in.high_f_q15, in.damping_q12, hp_state_));
  out2 = Clamp12(SVFLowpass(src, in.low_f_q15, in.damping_q12, lp_state_));
}

}  // namespace mtws
