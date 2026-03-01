#include "prex/mtws/engines/bender_engine.h"

namespace mtws {

namespace {
constexpr int32_t kUnityQ12 = 4096;

inline uint32_t U12ToQ12(uint16_t v) {
  if (v >= 4095U) return 4096U;
  return uint32_t(v);
}
}  // namespace

BenderEngine::BenderEngine(SineLUT* lut) : lut_(lut), phase_(0) {}

void BenderEngine::Init() {
  phase_ = 0;
}

void BenderEngine::OnSelected() {
  // Keep phase continuity.
}

void BenderEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  BenderControlFrame& out = frame.bender;
  out.phase_increment = global.pitch_inc;
  out.macro_x = global.macro_x;
  out.macro_y = global.macro_y;
  out.alt = global.mode_alt;
}

int32_t BenderEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

int32_t BenderEngine::SawQ12(uint32_t phase) {
  return int32_t((phase >> 20) & 0x0FFFU) - 2048;
}

int32_t BenderEngine::TriangleQ12(uint32_t phase) {
  uint32_t p = (phase >> 20) & 0x0FFFU;
  int32_t tri = (p < 2048U) ? int32_t((p << 1) - 2048) : int32_t((6143U - (p << 1)) - 2048);
  return Clamp12(tri);
}

int32_t BenderEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + int32_t((int64_t(b - a) * int32_t(t_q12)) >> 12);
}

int32_t BenderEngine::Fold12(int32_t x) {
  while (x > 2047 || x < -2048) {
    if (x > 2047) {
      x = 4095 - x;
    } else {
      x = -4096 - x;
    }
  }
  return x;
}

void BenderEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const BenderControlFrame& in = frame.bender;
  phase_ += in.phase_increment;

  uint32_t x_q12 = U12ToQ12(in.macro_x);
  uint32_t y_q12 = U12ToQ12(in.macro_y);

  if (!in.alt) {
    int32_t tri = TriangleQ12(phase_);
    int32_t sine = lut_->LookupLinear(phase_);

    int32_t drive_q12 = 4096 + int32_t((int64_t(x_q12) * 4));
    int32_t bias = int32_t(y_q12) - 2048;

    int32_t a = int32_t((int64_t(tri) * drive_q12) >> 12) + bias;
    int32_t b = int32_t((int64_t(sine) * drive_q12) >> 12) + bias;
    out1 = Fold12(a);
    out2 = Fold12(b);
    return;
  }

  int32_t ramp_up = SawQ12(phase_);
  int32_t tri_1 = TriangleQ12(phase_);

  uint32_t phase_shift = uint32_t(in.macro_y) << 20;
  uint32_t phase2 = phase_ + phase_shift;
  int32_t ramp_down = -SawQ12(phase2);
  int32_t tri_2 = TriangleQ12(phase2);

  out1 = Clamp12(LerpQ12(tri_1, ramp_up, x_q12));
  out2 = Clamp12(LerpQ12(tri_2, ramp_down, x_q12));
}

}  // namespace mtws
