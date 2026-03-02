#include "prex/bender/bender_solo_engine.h"

namespace {
// Converts knob domain (0..4095) to a Q12 interpolation domain (0..4096).
// The extra top code point allows exact unity interpolation at full-scale knob.
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }
}  // namespace

// Stores LUT dependency and initializes phase state.
BenderSoloEngine::BenderSoloEngine(mtws::SineLUT* lut) : lut_(lut), phase_(0) {}

// Resets oscillator phase to deterministic startup state.
void BenderSoloEngine::Init() {
  phase_ = 0;
}

// Copies control-frame values used at audio rate into a lightweight RenderFrame.
void BenderSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_increment = control.pitch_inc;
  out.macro_x = control.macro_x;
  out.macro_y = control.macro_y;
  out.alt = control.alt;
}

// Clamps to board audio range to avoid wrap/overflow at the DAC boundary.
int32_t BenderSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Reads the top 12 bits of phase to build a bipolar saw.
int32_t BenderSoloEngine::SawQ12(uint32_t phase) {
  return int32_t((phase >> 20) & 0x0FFFU) - 2048;
}

// Uses the same 12-bit phase slice and mirrors the second half-cycle into a triangle.
int32_t BenderSoloEngine::TriangleQ12(uint32_t phase) {
  uint32_t p = (phase >> 20) & 0x0FFFU;
  int32_t tri = (p < 2048U) ? int32_t((p << 1) - 2048) : int32_t((6143U - (p << 1)) - 2048);
  return Clamp12(tri);
}

// Interpolates between a and b using t_q12 in [0, 4096].
int32_t BenderSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + int32_t((int64_t(b - a) * int32_t(t_q12)) >> 12);
}

// Reflect-folds out-of-range samples until they are back in signed 12-bit bounds.
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

// Renders one stereo sample:
// - Normal mode: driven/folded triangle and sine with DC bias.
// - Alt mode: triangle-to-ramp crossfade with phase-offset stereo pairing.
void BenderSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_increment;

  uint32_t x_q12 = ToQ12(frame.macro_x);
  uint32_t y_q12 = ToQ12(frame.macro_y);

  if (!frame.alt) {
    int32_t tri = TriangleQ12(phase_);
    int32_t sine = lut_->LookupLinear(phase_);

    // `drive_q12` scales 1.0..5.0 as X goes 0..1, then Fold12 applies soft wavefold.
    int32_t drive_q12 = 4096 + int32_t((int64_t(x_q12) * 4));
    // `bias` is centered at zero so Y can shift folding symmetry.
    int32_t bias = int32_t(y_q12) - 2048;

    out1 = Fold12(int32_t((int64_t(tri) * drive_q12) >> 12) + bias);
    out2 = Fold12(int32_t((int64_t(sine) * drive_q12) >> 12) + bias);
    return;
  }

  int32_t ramp_up = SawQ12(phase_);
  int32_t tri_1 = TriangleQ12(phase_);

  // 12-bit knob value is moved into phase bits so one knob turn spans full cycle.
  uint32_t phase_shift = uint32_t(frame.macro_y) << 20;
  uint32_t phase2 = phase_ + phase_shift;
  int32_t ramp_down = -SawQ12(phase2);
  int32_t tri_2 = TriangleQ12(phase2);

  out1 = Clamp12(LerpQ12(tri_1, ramp_up, x_q12));
  out2 = Clamp12(LerpQ12(tri_2, ramp_down, x_q12));
}
