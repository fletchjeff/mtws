#include "prex/bender/bender_solo_engine.h"

namespace {
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }
}  // namespace

BenderSoloEngine::BenderSoloEngine(mtws::SineLUT* lut) : lut_(lut), phase_(0) {}

void BenderSoloEngine::Init() {
  phase_ = 0;
}

void BenderSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_increment = control.pitch_inc;
  out.macro_x = control.macro_x;
  out.macro_y = control.macro_y;
  out.alt = control.alt;
}

int32_t BenderSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

int32_t BenderSoloEngine::SawQ12(uint32_t phase) {
  return int32_t((phase >> 20) & 0x0FFFU) - 2048;
}

int32_t BenderSoloEngine::TriangleQ12(uint32_t phase) {
  uint32_t p = (phase >> 20) & 0x0FFFU;
  int32_t tri = (p < 2048U) ? int32_t((p << 1) - 2048) : int32_t((6143U - (p << 1)) - 2048);
  return Clamp12(tri);
}

int32_t BenderSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + int32_t((int64_t(b - a) * int32_t(t_q12)) >> 12);
}

int32_t BenderSoloEngine::Fold12(int32_t x) {
  while (x > 2047 || x < -2048) {
    if (x > 2047) {
      x = 4095 - x;
    } else {
      x = -4096 - x;
    }
  }
  return x;
}

void BenderSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_increment;

  uint32_t x_q12 = ToQ12(frame.macro_x);
  uint32_t y_q12 = ToQ12(frame.macro_y);

  if (!frame.alt) {
    int32_t tri = TriangleQ12(phase_);
    int32_t sine = lut_->LookupLinear(phase_);

    int32_t drive_q12 = 4096 + int32_t((int64_t(x_q12) * 4));
    int32_t bias = int32_t(y_q12) - 2048;

    out1 = Fold12(int32_t((int64_t(tri) * drive_q12) >> 12) + bias);
    out2 = Fold12(int32_t((int64_t(sine) * drive_q12) >> 12) + bias);
    return;
  }

  int32_t ramp_up = SawQ12(phase_);
  int32_t tri_1 = TriangleQ12(phase_);

  uint32_t phase_shift = uint32_t(frame.macro_y) << 20;
  uint32_t phase2 = phase_ + phase_shift;
  int32_t ramp_down = -SawQ12(phase2);
  int32_t tri_2 = TriangleQ12(phase2);

  out1 = Clamp12(LerpQ12(tri_1, ramp_up, x_q12));
  out2 = Clamp12(LerpQ12(tri_2, ramp_down, x_q12));
}
