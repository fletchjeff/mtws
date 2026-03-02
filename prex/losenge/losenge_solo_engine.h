#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// LosengeSoloEngine excites four band-pass filters from one carrier oscillator
// and mixes them into two outputs. Normal mode uses a saw carrier; alt uses a
// square carrier and alternate filter-frequency mapping.
class LosengeSoloEngine {
 public:
  // Control-rate frame consumed by RenderSample.
  struct RenderFrame {
    // Carrier phase increment in 0.32 phase units/sample.
    uint32_t phase_increment;
    // First band-pass coefficient for out1 in Q15.
    uint16_t out1_f1_q15;
    // Second band-pass coefficient for out1 in Q15.
    uint16_t out1_f2_q15;
    // First band-pass coefficient for out2 in Q15.
    uint16_t out2_f1_q15;
    // Second band-pass coefficient for out2 in Q15.
    uint16_t out2_f2_q15;
    // Shared damping/resonance term in Q12.
    uint16_t damping_q12;
    // Alternate carrier/mapping switch.
    bool alt;
  };

  // Creates the engine with LUT dependency (unused directly, retained for API parity).
  explicit LosengeSoloEngine(mtws::SineLUT* lut);

  // Resets oscillator and filter states.
  void Init();
  // Converts control values into per-block carrier/filter parameters.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // State-variable filter integrator memory.
  struct SVFState {
    // Low-pass integrator.
    int32_t lp;
    // Band-pass integrator.
    int32_t bp;
  };

  // Clamps to signed 12-bit board output domain.
  static int32_t Clamp12(int32_t v);
  // Converts frequency in Hz to SVF coefficient in Q15.
  static uint16_t HzToFQ15(uint32_t hz);
  // Integer linear interpolation helper with Q12 fraction.
  static uint32_t LerpU32(uint32_t a, uint32_t b, uint16_t t_u12);
  // Bipolar saw oscillator in signed 12-bit domain.
  static int32_t SawQ12(uint32_t phase);
  // Bipolar square oscillator in signed 12-bit domain.
  static int32_t SquareQ12(uint32_t phase);
  // One SVF update returning the band-pass output.
  static int32_t SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);

  // Kept for common engine interface.
  mtws::SineLUT* lut_;
  // Shared carrier phase accumulator in 0.32.
  uint32_t phase_;
  // out1 filter A state.
  SVFState out1_a_;
  // out1 filter B state.
  SVFState out1_b_;
  // out2 filter A state.
  SVFState out2_a_;
  // out2 filter B state.
  SVFState out2_b_;
};
