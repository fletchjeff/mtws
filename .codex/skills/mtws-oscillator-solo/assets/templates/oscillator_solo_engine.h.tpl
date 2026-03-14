#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// {{EngineClass}}SoloEngine is a small standalone scaffold for a new oscillator.
// Replace the sine-based placeholder DSP with the intended voice path once the
// control map and helper-reuse plan are locked.
class {{EngineClass}}SoloEngine {
 public:
  // Per-block parameters consumed by RenderSample().
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units per sample.
    uint32_t phase_increment;
    // Output gain in Q12 where 4096 = unity.
    uint16_t output_gain_q12;
    // Phase offset for Out2 in 0.32 phase units.
    uint32_t out2_phase_offset;
    // Alternate mode switch.
    bool alt;
  };

  // Creates the engine with a sine lookup dependency for the initial scaffold.
  explicit {{EngineClass}}SoloEngine(mtws::SineLUT* lut);

  // Resets runtime state to deterministic startup values.
  void Init();
  // Converts the shared solo control frame into a lightweight render frame.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in the board's signed 12-bit domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Clamps one sample to the signed 12-bit audio domain.
  static int32_t Clamp12(int32_t v);

  // Shared sine lookup table used by the placeholder oscillator path.
  mtws::SineLUT* lut_;
  // Phase accumulator in 0.32 format.
  uint32_t phase_;
};
