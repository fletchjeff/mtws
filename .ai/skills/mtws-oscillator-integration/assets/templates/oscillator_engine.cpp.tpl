#include "knots/src/engines/{{engine_name}}_engine.h"
#include "pico.h"

namespace mtws {

namespace {
// Converts the 0..4095 macro range into 0..4096 Q12.
inline uint32_t U12ToQ12(uint16_t v) {
  if (v >= 4095U) return 4096U;
  return uint32_t(v);
}

// Maps macro Y into a full-cycle phase offset for the starter scaffold.
inline uint32_t PhaseOffsetFromMacroY(uint16_t macro_y) {
  return uint32_t(macro_y) << 20;
}
}  // namespace

{{EngineClass}}Engine::{{EngineClass}}Engine(SineLUT* lut)
    : lut_(lut), phase_(0) {}

// Resets runtime phase to a deterministic startup state.
void {{EngineClass}}Engine::Init() {
  phase_ = 0;
}

// Keeps phase continuity when this slot is reselected.
void {{EngineClass}}Engine::OnSelected() {}

// Converts the shared mtws macro frame into the engine-specific control frame.
// Replace `{{engine_member}}` and `{{ControlFrameType}}` after adding the new
// control-frame struct to `knots/src/core/shared_state.h`.
void {{EngineClass}}Engine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  {{ControlFrameType}}& out = frame.{{engine_member}};
  out.phase_increment = global.pitch_inc;
  out.output_gain_q12 = uint16_t(U12ToQ12(global.macro_x));
  out.out2_phase_offset = PhaseOffsetFromMacroY(global.macro_y);
  out.alt = global.mode_alt;
}

// Clamps one sample to the board's signed 12-bit audio range.
int32_t __not_in_flash_func({{EngineClass}}Engine::Clamp12)(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Renders a small audible scaffold while the final oscillator DSP is still
// being ported. Replace `{{engine_member}}` and the placeholder sine path as
// soon as the real engine-specific control frame is in place.
void __not_in_flash_func({{EngineClass}}Engine::RenderSample)(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const {{ControlFrameType}}& in = frame.{{engine_member}};

  phase_ += in.phase_increment;

  int32_t base = lut_->LookupLinear(phase_);
  int32_t shifted = lut_->LookupLinear(phase_ + in.out2_phase_offset);
  if (in.alt) {
    shifted = -shifted;
  }

  out1 = Clamp12(int32_t((int64_t(base) * in.output_gain_q12) >> 12));
  out2 = Clamp12(int32_t((int64_t(shifted) * in.output_gain_q12) >> 12));
}

}  // namespace mtws
