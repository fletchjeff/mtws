#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// DinSumSoloEngine is a filtered-noise engine. It produces either dual-bandpass
// outputs (normal) or split high/low outputs (alt) using fixed-point SVF stages.
class DinSumSoloEngine {
 public:
  // Per-block parameters prepared on the control thread.
  struct RenderFrame {
    // Low filter frequency coefficient in Q15.
    uint16_t low_f_q15;
    // High filter frequency coefficient in Q15.
    uint16_t high_f_q15;
    // Resonance damping factor in Q12.
    uint16_t damping_q12;
    // Alternate routing/mode switch.
    bool alt;
  };

  // Creates the engine with sine LUT dependency (unused here but kept uniform).
  explicit DinSumSoloEngine(mtws::SineLUT* lut);

  // Resets noise generator and filter states.
  void Init();
  // Builds filter coefficients and mode flags from one control frame.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output range.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // State-variable filter integrator state.
  struct SVFState {
    // Low-pass integrator state.
    int32_t lp;
    // Band-pass integrator state.
    int32_t bp;
  };

  // Clamps to signed 12-bit board audio range.
  static int32_t Clamp12(int32_t v);
  // Converts frequency in Hz to normalized SVF coefficient f in Q15.
  static uint16_t HzToFQ15(uint32_t hz);
  // Integer interpolation utility with Q12 fraction.
  static uint32_t LerpU32(uint32_t a, uint32_t b, uint16_t t_u12);
  // One SVF update that returns high-pass output.
  static int32_t SVFHighpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);
  // One SVF update that returns low-pass output.
  static int32_t SVFLowpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);
  // One SVF update that returns band-pass output.
  static int32_t SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);

  // Kept for interface consistency with other solo engines.
  mtws::SineLUT* lut_;
  // Xorshift PRNG state for white-noise source.
  uint32_t noise_state_;
  // One-pole low-pass memory used to derive pink-ish noise.
  int32_t pink_state_;
  // Normal-mode band-pass state for out1.
  SVFState bp1_state_;
  // Normal-mode band-pass state for out2.
  SVFState bp2_state_;
  // Alt-mode high-pass state for out1.
  SVFState hp_state_;
  // Alt-mode low-pass state for out2.
  SVFState lp_state_;
};
