#include "prex/losenge/losenge_solo_engine.h"

namespace {
inline uint16_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : v; }
}  // namespace

LosengeSoloEngine::LosengeSoloEngine(mtws::SineLUT* lut)
    : lut_(lut), phase_(0), out1_a_{0, 0}, out1_b_{0, 0}, out2_a_{0, 0}, out2_b_{0, 0} {
  Init();
}

void LosengeSoloEngine::Init() {
  (void)lut_;
  phase_ = 0;
  out1_a_ = {0, 0};
  out1_b_ = {0, 0};
  out2_a_ = {0, 0};
  out2_b_ = {0, 0};
}

int32_t LosengeSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

uint16_t LosengeSoloEngine::HzToFQ15(uint32_t hz) {
  if (hz < 20U) hz = 20U;
  if (hz > 10000U) hz = 10000U;
  uint32_t f_q15 = (hz * 32768U + 12000U) / 24000U;
  if (f_q15 > 32767U) f_q15 = 32767U;
  if (f_q15 < 32U) f_q15 = 32U;
  return uint16_t(f_q15);
}

uint32_t LosengeSoloEngine::LerpU32(uint32_t a, uint32_t b, uint16_t t_u12) {
  uint32_t t_q12 = (t_u12 >= 4095U) ? 4096U : uint32_t(t_u12);
  return uint32_t(int64_t(a) + ((int64_t(b) - int64_t(a)) * int64_t(t_q12) >> 12));
}

int32_t LosengeSoloEngine::SawQ12(uint32_t phase) {
  return int32_t((phase >> 20) & 0x0FFFU) - 2048;
}

int32_t LosengeSoloEngine::SquareQ12(uint32_t phase) {
  return (phase < 0x80000000U) ? 2047 : -2048;
}

int32_t LosengeSoloEngine::SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state) {
  int32_t hp = in - state.lp - int32_t((int64_t(damping_q12) * state.bp) >> 12);
  state.bp += int32_t((int64_t(f_q15) * hp) >> 15);
  state.lp += int32_t((int64_t(f_q15) * state.bp) >> 15);
  return state.bp;
}

void LosengeSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_increment = control.pitch_inc;
  out.alt = control.alt;
  out.damping_q12 = uint16_t(1800U + ((uint32_t(4095U - control.macro_x) * 1200U) >> 12));

  uint16_t x = ToQ12(control.macro_x);
  uint16_t y = ToQ12(control.macro_y);

  uint32_t o1_f1_hz;
  uint32_t o1_f2_hz;
  uint32_t o2_f1_hz;
  uint32_t o2_f2_hz;

  if (!out.alt) {
    o1_f1_hz = LerpU32(500U, 400U, x);
    o1_f2_hz = LerpU32(1700U, 800U, x);
    o2_f1_hz = LerpU32(700U, 300U, y);
    o2_f2_hz = LerpU32(1100U, 2200U, y);
  } else {
    o1_f1_hz = LerpU32(350U, 400U, x);
    o1_f2_hz = LerpU32(600U, 800U, x);
    o2_f1_hz = LerpU32(700U, 500U, y);
    o2_f2_hz = LerpU32(1100U, 1700U, y);
  }

  out.out1_f1_q15 = HzToFQ15(o1_f1_hz);
  out.out1_f2_q15 = HzToFQ15(o1_f2_hz);
  out.out2_f1_q15 = HzToFQ15(o2_f1_hz);
  out.out2_f2_q15 = HzToFQ15(o2_f2_hz);
}

void LosengeSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_increment;
  int32_t carrier = frame.alt ? SquareQ12(phase_) : SawQ12(phase_);

  int32_t o1a = SVFBandpass(carrier, frame.out1_f1_q15, frame.damping_q12, out1_a_);
  int32_t o1b = SVFBandpass(carrier, frame.out1_f2_q15, frame.damping_q12, out1_b_);
  int32_t o2a = SVFBandpass(carrier, frame.out2_f1_q15, frame.damping_q12, out2_a_);
  int32_t o2b = SVFBandpass(carrier, frame.out2_f2_q15, frame.damping_q12, out2_b_);

  out1 = Clamp12((o1a + o1b) >> 1);
  out2 = Clamp12((o2a + o2b) >> 1);
}
