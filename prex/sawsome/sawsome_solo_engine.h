#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// SawsomeSoloEngine is a 7-voice supersaw/supertriangle oscillator with
// per-voice detune and stereo panning. It uses PolyBLEP correction to reduce
// aliasing around waveform discontinuities.
class SawsomeSoloEngine {
 public:
  // Per-block voice parameters consumed by audio rendering.
  struct RenderFrame {
    // Per-voice oscillator increments in 0.32 phase units/sample.
    uint32_t phase_increment[7];
    // Per-voice left gain in Q12.
    int16_t gain_l_q12[7];
    // Per-voice right gain in Q12.
    int16_t gain_r_q12[7];
    // Alternate waveform switch: false=saw, true=triangle.
    bool alt;
  };

  // Creates engine with LUT dependency (kept for common engine signature).
  explicit SawsomeSoloEngine(mtws::SineLUT* lut);

  // Resets voice phases and triangle integrator states.
  void Init();
  // Builds detune and stereo gain maps from one control frame.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit range.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Clamps to signed 12-bit output domain.
  static int32_t Clamp12(int32_t v);
  // Band-limited saw oscillator using PolyBLEP edge correction.
  static int32_t PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc);
  // Integrates band-limited saw into triangle for one voice.
  int32_t PolyBlepTriangleQ12(int voice_index, uint32_t phase, uint32_t phase_inc);

  // Kept for API consistency with other solo engines.
  mtws::SineLUT* lut_;
  // Per-voice phase accumulators in 0.32 format.
  uint32_t phases_[7];
  // Per-voice leaky integrator state used for triangle synthesis.
  int32_t tri_state_[7];
};
