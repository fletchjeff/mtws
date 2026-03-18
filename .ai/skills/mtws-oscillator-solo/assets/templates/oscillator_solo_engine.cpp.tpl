#include "knots/solo_engines/{{engine_name}}/{{engine_name}}_solo_engine.h"

namespace {
// Converts a 0..4095 control into the Q12 range 0..4096.
inline uint32_t U12ToQ12(uint16_t v) {
  if (v >= 4095U) return 4096U;
  return uint32_t(v);
}

// Maps macro Y into a full-cycle phase offset. The left shift places the knob's
// 12-bit range into the upper phase bits so the full control span covers nearly
// one full lookup-table cycle.
inline uint32_t PhaseOffsetFromMacroY(uint16_t macro_y) {
  return uint32_t(macro_y) << 20;
}
}  // namespace

// Stores the lookup-table dependency and initializes runtime phase state.
{{EngineClass}}SoloEngine::{{EngineClass}}SoloEngine(mtws::SineLUT* lut)
    : lut_(lut), phase_(0) {}

// Resets the phase accumulator so the standalone patch starts deterministically.
void {{EngineClass}}SoloEngine::Init() {
  phase_ = 0;
}

// Copies control-rate values into a lighter render frame. Replace this simple
// mapping with the intended oscillator-specific control conversion as needed.
void {{EngineClass}}SoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_increment = control.pitch_inc;
  out.output_gain_q12 = uint16_t(U12ToQ12(control.macro_x));
  out.out2_phase_offset = PhaseOffsetFromMacroY(control.macro_y);
  out.alt = control.alt;
}

// Clamps one sample to the board's signed 12-bit output range.
int32_t {{EngineClass}}SoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Renders a simple two-output sine scaffold so the target is audible quickly.
// Out1 follows the base oscillator. Out2 uses a controllable phase offset and
// flips polarity in alt mode so the first hardware check exercises both outputs.
void {{EngineClass}}SoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_increment;

  int32_t base = lut_->LookupLinear(phase_);
  int32_t shifted = lut_->LookupLinear(phase_ + frame.out2_phase_offset);
  if (frame.alt) {
    shifted = -shifted;
  }

  out1 = Clamp12(int32_t((int64_t(base) * frame.output_gain_q12) >> 12));
  out2 = Clamp12(int32_t((int64_t(shifted) * frame.output_gain_q12) >> 12));
}
