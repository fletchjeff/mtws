#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// DinSumSoloEngine is a bounded sine-to-saw morph voice. In normal mode, both
// outputs choose a random interpolation point inside the interval between the
// shared sine and their saw targets, with Y crossfading the `p` noise source
// between smooth low-passed and nervous high-passed variants. Out1 uses the
// local ramp-up saw, while Out2 uses the same ramp-up saw evaluated 90 degrees
// later. In alt mode, the oscillator becomes cycle-locked and
// probabilistic: each phase wrap chooses either sine or saw for the whole
// cycle, with saw probability set by X and decision stickiness set by Y.
class DinSumSoloEngine {
 public:
  // Per-block parameters prepared on the control thread.
  struct RenderFrame {
    // Shared phase increment for the phase-aligned sine and saw oscillators.
    uint32_t phase_increment;
    // Precomputed PolyBLEP reciprocal: `(1 << 36) / phase_increment`.
    // This lets the audio path use multiply-based BLEP correction.
    uint32_t phase_inc_recip_q24;
    // Morph knob in Q12 where 0 = pure sine and 4096 = pure saw.
    uint16_t morph_q12;
    // Coherence control in Q12 where 0 = stable/sticky and 4096 = nervous.
    uint16_t coherence_q12;
    // Previous bipolar low-passed random target shared by both outputs.
    int16_t random_lp_prev;
    // Next bipolar low-passed random target shared by both outputs.
    int16_t random_lp_next;
    // Previous bipolar high-passed random target shared by both outputs.
    int16_t random_hp_prev;
    // Next bipolar high-passed random target shared by both outputs.
    int16_t random_hp_next;
    // Alternate mode enables cycle-locked probabilistic selection instead of
    // bounded interpolation.
    bool alt;
    // Monotonic frame sequence used to reset audio-rate random interpolation
    // whenever the control core publishes a fresh 16-sample control block.
    uint32_t frame_epoch;
  };

  // Creates the engine with the shared sine LUT dependency.
  explicit DinSumSoloEngine(mtws::SineLUT* lut);

  // Resets phase, PRNG, and control/audio interpolation state.
  void Init();
  // Builds one control-rate render frame from the latest control snapshot.
  // Inputs: pitch increment and macro values from the solo control router.
  // Outputs: phase increment, morph position, and the next pair of random
  // targets used by the audio-rate bounded interpolation.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out);
  // Renders one stereo sample in signed 12-bit output range.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Clamps to signed 12-bit board audio range.
  static int32_t Clamp12(int32_t v);
  // Interpolates between two signed audio samples with a Q12 morph amount.
  // Inputs: endpoints `a` and `b`, and `t_q12` in the range 0..4096.
  // Output: a bounded point inside the interval [a, b].
  static int32_t LerpQ12(int32_t a, int32_t b, uint16_t t_q12);
  // Generates a band-limited saw from the shared phase accumulator.
  // Inputs: phase and phase increment in the native 32-bit oscillator domain.
  // Output: saw sample in the signed 12-bit audio range.
  static int32_t PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc, uint32_t recip_q24);
  // Advances the control-rate PRNG and returns a bipolar 12-bit random value.
  // Input/output: xorshift32 state stored in `state`.
  // Output: signed value approximately in the range -2048..2047.
  static int16_t Rand12Bipolar(uint32_t& state);
  // Advances the audio-rate PRNG and returns a unipolar 12-bit random value.
  // Input/output: xorshift32 state stored in `state`.
  // Output: unsigned value in the range 0..4095 for probability comparisons.
  static uint16_t Rand12Unipolar(uint32_t& state);

  // Kept for interface consistency with other solo engines.
  mtws::SineLUT* lut_;
  // Xorshift PRNG state used by the control-rate random-target generator.
  uint32_t random_state_;
  // Xorshift PRNG state used by the audio-rate cycle-choice generator in alt.
  uint32_t alt_random_state_;
  // Shared phase accumulator used for both source waveforms.
  uint32_t phase_;
  // Most recently generated low-passed random target shared by both outputs.
  int16_t control_random_lp_next_;
  // Most recently generated high-passed random target shared by both outputs.
  int16_t control_random_hp_next_;
  // Monotonic counter written into each render frame so the audio path can
  // restart its 16-sample random interpolation when the frame changes.
  uint32_t frame_epoch_counter_;
  // The last frame sequence observed on the audio core.
  uint32_t active_frame_epoch_;
  // Audio-rate interpolation step inside the current 16-sample control block.
  uint8_t random_interp_step_;
  // Tracks whether the audio path is currently rendering alt-mode cycles.
  bool active_alt_mode_;
  // Stores the waveform family selected for Out1 on the current alt-mode cycle.
  bool alt_cycle_is_saw_out1_;
  // Stores the waveform family selected for Out2 on the current alt-mode cycle.
  bool alt_cycle_is_saw_out2_;
  // Counts how many wraps have elapsed since the current Out1 alt-cycle choice
  // was last refreshed. This implements Y-controlled stickiness.
  uint8_t alt_cycle_hold_out1_;
  // Counts how many wraps have elapsed since the current Out2 alt-cycle choice
  // was last refreshed. This implements Y-controlled stickiness.
  uint8_t alt_cycle_hold_out2_;
};
