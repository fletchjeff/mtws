#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// BenderSoloEngine generates two related wave-shaping voices from one phase
// accumulator. The current reset pass reduces the engine to one sine-LUT voice
// with two parallel evaluators: a Utility-Pair-style antialiased wavefolder on
// Out1 and a Utility-Pair-style bitcrusher on Out2.
class BenderSoloEngine {
 public:
  // Precomputed per-block parameters consumed at audio rate.
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units/sample.
    uint32_t phase_increment;
    // Fold drive in Q7 where 128 = 1.0x input amplitude.
    uint16_t fold_drive_q7;
    // Dry/wet fold mix in Q12 where 0 = dry sine and 4096 = fully folded.
    uint16_t fold_mix_q12;
    // Bitcrusher step size in signed-12-bit sample units.
    int16_t crush_amount;
    // Alternate routing switch.
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
  // Utility-Pair fold function: triangle-shaped transfer with 8192-sample period.
  static int32_t FoldFunction(int32_t x);
  // Integral of the fold function used for antiderivative antialiasing.
  static int32_t FoldIntegral(int32_t x);
  // Derivative-based antialiased fold using the provided previous input state.
  static int32_t AntiAliasedFold(int32_t x, int32_t& last_integral, int32_t& last_input);
  // Utility-Pair quantizer/bitcrusher transfer.
  static int32_t CrushFunction(int32_t x, int32_t amount);
  // Linear interpolation in Q12 for dry/wet output mixing.
  static int32_t LerpQ12(int32_t dry, int32_t wet, uint32_t mix_q12);

  // Shared sine lookup table (owned by caller).
  mtws::SineLUT* lut_;
  // Oscillator phase accumulator in 0.32 format.
  uint32_t phase_;
  // Previous fold integral sample for the sine->fold path.
  int32_t last_fold_integral_pre_;
  // Previous fold input sample for the sine->fold path.
  int32_t last_fold_input_pre_;
  // Previous fold integral sample for the crush->fold path.
  int32_t last_fold_integral_post_;
  // Previous fold input sample for the crush->fold path.
  int32_t last_fold_input_post_;
  // Tracks the last mode so fold histories can be reset on routing changes.
  bool last_alt_;
};
