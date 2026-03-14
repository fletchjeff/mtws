#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// LosengeSoloEngine is a pitched formant-oscillator bank inspired by the
// Braids/Twists VOWL model. One hidden carrier resets two independent
// three-formant output banks once per cycle, matching the current integrated
// `mtws` engine behavior.
class LosengeSoloEngine {
 public:
  // Control-rate frame consumed by RenderSample.
  struct RenderFrame {
    // Carrier phase increment in 0.32 phase units/sample.
    uint32_t phase_increment;
    // Per-output, per-formant oscillator increments in 0.32 phase units/sample.
    uint32_t formant_increment[2][3];
    // Per-output, per-formant amplitudes in Q12, derived from the vowel table.
    uint16_t formant_amplitude_q12[2][3];
    // Output gain after glottal-envelope shaping in Q12.
    uint16_t voice_gain_q12;
  };

  // Creates the engine with sine LUT dependency used by the first two formants.
  explicit LosengeSoloEngine(mtws::SineLUT* lut);

  // Resets carrier phase and all formant oscillator phases.
  void Init();
  // Converts controls into pitch, vowel interpolation, and a shared tract-size
  // shift. Alt mode swaps from the base F1/F2/F3 formant set to a brighter
  // upper F2/F3/F4 set.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Clamps to signed 12-bit board output domain.
  static int32_t Clamp12(int32_t v);
  // Converts formant frequency in Hz to 0.32 phase increment at 48kHz.
  // Uses the same constant-multiply approximation as the integrated engine.
  static uint32_t HzToPhaseIncrement(uint32_t hz);
  // Applies a Q12 global tract/formant shift to a base formant frequency in Hz.
  static uint32_t ApplyShiftQ12(uint16_t hz, uint16_t shift_q12);
  // Bipolar square oscillator in signed 12-bit domain for the third formant.
  static int32_t SquareQ12(uint32_t phase);
  // Mixes three formant oscillators with Q12 gains into the board output domain.
  static int32_t MixFormantsQ12(int32_t f1,
                                int32_t f2,
                                int32_t f3,
                                const uint16_t (&amplitudes_q12)[3],
                                uint16_t envelope_q12,
                                uint16_t voice_gain_q12);

  // Shared sine lookup table used by the sine formants.
  mtws::SineLUT* lut_;
  // Carrier phase accumulator in 0.32. It defines the pitch and reset period.
  uint32_t phase_;
  // Per-output independent formant oscillator phases in 0.32, reset on carrier wrap.
  uint32_t formant_phase_[2][3];
};
