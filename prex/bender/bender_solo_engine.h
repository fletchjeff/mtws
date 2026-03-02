#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// BenderSoloEngine generates two related wave-shaping voices from one phase
// accumulator. The control path builds a per-block RenderFrame, and the audio
// path only consumes cached fixed-point values to keep ProcessSample() cheap.
class BenderSoloEngine {
 public:
  // Precomputed per-block parameters consumed at audio rate.
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units/sample.
    uint32_t phase_increment;
    // Macro X from control router in 0..4095.
    uint16_t macro_x;
    // Macro Y from control router in 0..4095.
    uint16_t macro_y;
    // Alternate algorithm switch.
    bool alt;
  };

  // Creates the engine with a sine lookup table dependency used by RenderSample.
  explicit BenderSoloEngine(mtws::SineLUT* lut);

  // Resets runtime state (phase accumulator).
  void Init();
  // Converts control-domain values into a RenderFrame for the current block.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in the board's signed 12-bit domain (-2048..2047).
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Hard-clamps a sample to signed 12-bit audio range.
  static int32_t Clamp12(int32_t v);
  // Generates a bipolar triangle wave in signed 12-bit range from 0.32 phase.
  static int32_t TriangleQ12(uint32_t phase);
  // Generates a bipolar saw wave in signed 12-bit range from 0.32 phase.
  static int32_t SawQ12(uint32_t phase);
  // Linear interpolation with 12-bit fraction where t_q12 is 0..4096.
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  // Reflective fold that keeps values in signed 12-bit range.
  static int32_t Fold12(int32_t x);

  // Shared sine lookup table (owned by caller).
  mtws::SineLUT* lut_;
  // Oscillator phase accumulator in 0.32 format.
  uint32_t phase_;
};
