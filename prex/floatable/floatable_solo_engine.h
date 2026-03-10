#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// FloatableSoloEngine renders two finished 256-sample output tables from the
// shared curated bank set on core 1, then the audio loop phase-reads those
// completed tables.
class FloatableSoloEngine {
 public:
  // Per-block values used during audio-rate rendering.
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units/sample.
    uint32_t phase_inc;
    // Final rendered wavetable for Out1 in signed 16-bit source-wave domain.
    int16_t rendered_out1[256];
    // Final rendered wavetable for Out2 in signed 16-bit source-wave domain.
    int16_t rendered_out2[256];
  };

  // Creates the engine with shared LUT dependency (kept for interface parity).
  explicit FloatableSoloEngine(mtws::SineLUT* lut);

  // Resets oscillator phase.
  void Init();
  // Builds the next pair of final output tables from the latest control frame.
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
