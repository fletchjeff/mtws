#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// FloatableSoloEngine reads from a 64-wave wavetable bank and crossfades
// neighboring tables along X. Alt mode switches between two full banks.
class FloatableSoloEngine {
 public:
  // Per-block values used during audio-rate rendering.
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units/sample.
    uint32_t phase_inc;
    // Left/source wavetable row index (0..63).
    uint8_t i00;
    // Right/next wavetable row index (0..63, wrapped in row).
    uint8_t i10;
    // Horizontal table crossfade fraction in Q12 (0..4096).
    uint16_t x_frac_q12;
    // Selects wavetable bank B when true, bank A when false.
    bool alt;
  };

  // Creates the engine with shared LUT dependency (kept for interface parity).
  explicit FloatableSoloEngine(mtws::SineLUT* lut);

  // Resets oscillator phase.
  void Init();
  // Maps macro controls to table indices and interpolation fractions.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Q12 interpolation helper where t_q12 is 0..4096.
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);

  // Kept for API consistency across engines.
  mtws::SineLUT* lut_;
  // Oscillator phase accumulator in 0.32 format.
  uint32_t phase_;
};
