#include "prex/din_sum/din_sum_solo_engine.h"

namespace {
// Converts knob range 0..4095 to the local Q12 convention 0..4096.
inline uint16_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : v; }

// One control frame spans 16 audio samples in SoloAppBase.
constexpr uint8_t kRandomInterpolationSteps = 16U;
// Since the interpolation length is 16 samples, `>> 4` converts a difference
// scaled by the current step index back into the native random-value domain.
constexpr uint8_t kRandomInterpolationShift = 4U;
// The low-passed random target is updated once per 16-sample control frame.
// A shift of 2 means the new target moves 25% of the way toward the latest
// uniform random input on each control update. A shift of 1 was considered, but
// it moved too quickly to feel meaningfully more filtered than the raw target.
// A shift of 3 was smoother still, but reduced the liveliness of the normal
// morph too much.
constexpr uint8_t kFilteredRandomShift = 2U;
// Unity for the local Q12 morph convention.
constexpr uint16_t kUnityQ12 = 4096U;
// Halfway point in the local Q12 morph convention. This is the maximum legal
// deviation range because the bounded morph corridor shrinks symmetrically from
// both ends toward the center.
constexpr uint16_t kHalfQ12 = 2048U;
// Alt-mode wrap stickiness spans 1..8 cycles. Low Y holds the same waveform
// family longer; high Y rerolls almost every wrap.
constexpr uint8_t kAltHoldCyclesMin = 1U;
constexpr uint8_t kAltHoldCyclesMax = 8U;
}  // namespace

// Creates the oscillator and initializes the shared sine LUT dependency.
// Input: pointer to the global LUT object owned by SoloAppBase.
// Output: an engine ready to reset and render.
DinSumSoloEngine::DinSumSoloEngine(mtws::SineLUT* lut)
    : lut_(lut),
      random_state_(1U),
      alt_random_state_(0x6d2b79f5U),
      phase_(0U),
      control_random_lp_next_(0),
      control_random_hp_next_(0),
      frame_epoch_counter_(0U),
      active_frame_epoch_(0U),
      random_interp_step_(0U),
      active_alt_mode_(false),
      alt_cycle_is_saw_out1_(false),
      alt_cycle_is_saw_out2_(false),
      alt_cycle_hold_out1_(0U),
      alt_cycle_hold_out2_(0U) {
  Init();
}

// Resets oscillator phase, random-target state, and the audio-rate interpolation
// cursor to deterministic startup values.
// Inputs: none.
// Outputs: internal state set to the initial pure-sine-ready condition.
void DinSumSoloEngine::Init() {
  (void)lut_;
  random_state_ = 1U;
  alt_random_state_ = 0x6d2b79f5U;
  phase_ = 0U;
  control_random_lp_next_ = 0;
  control_random_hp_next_ = 0;
  frame_epoch_counter_ = 0U;
  active_frame_epoch_ = 0U;
  random_interp_step_ = 0U;
  active_alt_mode_ = false;
  alt_cycle_is_saw_out1_ = false;
  alt_cycle_is_saw_out2_ = false;
  alt_cycle_hold_out1_ = 0U;
  alt_cycle_hold_out2_ = 0U;
}

// Clamps any intermediate sample to the signed 12-bit ComputerCard audio range.
// Input: unconstrained signed sample.
// Output: sample limited to -2048..2047.
int32_t DinSumSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Interpolates between two signed samples with a Q12 morph amount.
// Inputs: endpoints `a` and `b`, and `t_q12` in the range 0..4096.
// Output: a point guaranteed to stay inside the closed interval [a, b].
int32_t DinSumSoloEngine::LerpQ12(int32_t a, int32_t b, uint16_t t_q12) {
  uint32_t t = t_q12;
  if (t > kUnityQ12) t = kUnityQ12;
  return int32_t(int64_t(a) + ((int64_t(b) - int64_t(a)) * int64_t(t) >> 12));
}

// Generates a PolyBLEP-corrected saw from the shared phase accumulator.
// Inputs: current phase and phase increment in the native 32-bit phase domain.
// Output: a band-limited saw in the signed 12-bit audio range.
int32_t DinSumSoloEngine::PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc) {
  int32_t saw = int32_t((phase >> 20) & 0x0FFFU) - 2048;
  if (phase_inc == 0U) return saw;

  if (phase < phase_inc) {
    // Leading discontinuity correction. The numerator/denominator pair maps the
    // current phase position inside the BLEP window into Q12 0..4096. Q12 is
    // used here because it keeps the correction accurate enough while staying
    // in cheap 32-bit math. A wider Q domain would cost more CPU for little
    // audible benefit in this single-oscillator context.
    uint32_t t_q12 = uint32_t((uint64_t(phase) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = x + x - int32_t((int64_t(x) * x) >> 12) - 4096;
    saw -= corr >> 1;
  }

  uint32_t tail = 0xFFFFFFFFU - phase;
  if (tail < phase_inc) {
    // Trailing discontinuity correction using the same Q12 BLEP window.
    uint32_t t_q12 = uint32_t((uint64_t(tail) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = int32_t((int64_t(x) * x) >> 12);
    saw += corr >> 1;
  }

  return Clamp12(saw);
}

// Advances the control-rate xorshift generator and returns a bipolar 12-bit
// random target suitable for bounded morph modulation.
// Input/output: mutable PRNG state.
// Output: signed value approximately in the range -2048..2047.
int16_t DinSumSoloEngine::Rand12Bipolar(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return int16_t(((state >> 20) & 0x0FFFU) - 2048);
}

// Advances the xorshift generator and returns a unipolar 12-bit random value.
// Input/output: mutable PRNG state.
// Output: unsigned value in the range 0..4095.
uint16_t DinSumSoloEngine::Rand12Unipolar(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return uint16_t((state >> 20) & 0x0FFFU);
}

// Builds one control-rate frame. This captures the current phase increment and
// morph knobs, then advances the shared low-pass and high-pass random targets
// used for the next 16-sample audio interpolation span.
// Inputs: pitch/macro values from the solo control router.
// Outputs: one immutable render frame for the audio core.
void DinSumSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) {
  const uint16_t morph_q12 = ToQ12(control.macro_x);
  const uint16_t coherence_q12 = ToQ12(control.macro_y);
  const int16_t random_lp_prev = control_random_lp_next_;
  const int16_t random_hp_prev = control_random_hp_next_;
  const int16_t random_raw = Rand12Bipolar(random_state_);
  const int16_t random_lp_next = int16_t(
      int32_t(random_lp_prev) + ((int32_t(random_raw) - int32_t(random_lp_prev)) >> kFilteredRandomShift));

  // The high-passed branch is the residual between the raw target and the
  // low-passed branch. Clamping keeps it inside the same nominal bipolar range
  // as the other random targets so the later corridor math still behaves.
  int32_t random_hp_next = int32_t(random_raw) - int32_t(random_lp_next);
  if (random_hp_next < -2048) random_hp_next = -2048;
  if (random_hp_next > 2047) random_hp_next = 2047;

  control_random_lp_next_ = random_lp_next;
  control_random_hp_next_ = int16_t(random_hp_next);
  ++frame_epoch_counter_;

  out.phase_increment = control.pitch_inc;
  out.morph_q12 = morph_q12;
  out.coherence_q12 = coherence_q12;
  out.random_lp_prev = random_lp_prev;
  out.random_lp_next = random_lp_next;
  out.random_hp_prev = random_hp_prev;
  out.random_hp_next = int16_t(random_hp_next);
  out.alt = control.alt;
  out.frame_epoch = frame_epoch_counter_;
}

// Renders one stereo sample. Out1 uses the bounded random interpolation between
// the shared sine and the local upward saw. Out2 uses the same saw direction,
// but evaluated 90 degrees later so the full-saw endpoints no longer collapse
// to silence in mono. In normal mode, Y crossfades the `p` source for Out1
// from low-passed to high-passed random, while Out2 uses the inverse crossfade
// against its half-cycle-shifted saw target. In alt mode, both outputs become
// cycle-locked and choose either sine or saw once per phase wrap, with saw
// probability set directly by the morph control.
// Inputs: immutable render frame prepared at control rate.
// Outputs: two signed 12-bit audio samples.
void DinSumSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  if (frame.frame_epoch != active_frame_epoch_) {
    active_frame_epoch_ = frame.frame_epoch;
    random_interp_step_ = 0U;
  }

  const uint32_t previous_phase = phase_;
  phase_ += frame.phase_increment;
  const bool wrapped = phase_ < previous_phase;

  const int32_t sine = lut_->LookupLinear(phase_);
  const int32_t saw_up = PolyBlepSawQ12(phase_, frame.phase_increment);
  // A 90-degree phase offset is 0x40000000 in the native 32-bit phase domain.
  // Reusing the same PolyBLEP generator keeps both outputs on the same
  // anti-aliased saw implementation while moving only Out2's saw endpoint.
  const int32_t saw_up_shifted = PolyBlepSawQ12(phase_ + 0x40000000U, frame.phase_increment);

  if (frame.alt) {
    // Low Y increases hold length so waveform-family decisions persist across
    // several wraps. High Y reduces the hold toward one cycle so the alt mode
    // flickers more aggressively.
    const uint8_t hold_cycles = uint8_t(
        kAltHoldCyclesMax -
        ((uint32_t(frame.coherence_q12) * uint32_t(kAltHoldCyclesMax - kAltHoldCyclesMin) + 2048U) >> 12));

    // Entering alt mode always starts a fresh choice. On later wraps, only
    // reroll if the current choice has been held for the requested number of
    // cycles.
    if (!active_alt_mode_) {
      active_alt_mode_ = true;
      alt_cycle_is_saw_out1_ = Rand12Unipolar(alt_random_state_) < frame.morph_q12;
      alt_cycle_is_saw_out2_ = Rand12Unipolar(alt_random_state_) < frame.morph_q12;
      alt_cycle_hold_out1_ = 0U;
      alt_cycle_hold_out2_ = 0U;
    } else if (wrapped) {
      if (++alt_cycle_hold_out1_ >= hold_cycles) {
        alt_cycle_is_saw_out1_ = Rand12Unipolar(alt_random_state_) < frame.morph_q12;
        alt_cycle_hold_out1_ = 0U;
      }
      if (++alt_cycle_hold_out2_ >= hold_cycles) {
        alt_cycle_is_saw_out2_ = Rand12Unipolar(alt_random_state_) < frame.morph_q12;
        alt_cycle_hold_out2_ = 0U;
      }
    }
    out1 = alt_cycle_is_saw_out1_ ? saw_up : sine;
    out2 = alt_cycle_is_saw_out2_ ? saw_up_shifted : sine;
    return;
  }
  active_alt_mode_ = false;
  alt_cycle_hold_out1_ = 0U;
  alt_cycle_hold_out2_ = 0U;

  // The interpolated random value is audio-rate smooth but only refreshed once
  // per 16-sample control block. The `>> 4` shift matches the 16-sample block
  // length exactly. A stepped sample-and-hold variant was considered, but the
  // interpolated form preserves pitch better and avoids obvious zippering.
  const int32_t random_lp_interp = int32_t(frame.random_lp_prev) +
      (((int32_t(frame.random_lp_next) - int32_t(frame.random_lp_prev)) * int32_t(random_interp_step_)) >>
       kRandomInterpolationShift);
  const int32_t random_hp_interp = int32_t(frame.random_hp_prev) +
      (((int32_t(frame.random_hp_next) - int32_t(frame.random_hp_prev)) * int32_t(random_interp_step_)) >>
       kRandomInterpolationShift);
  if (random_interp_step_ < (kRandomInterpolationSteps - 1U)) {
    ++random_interp_step_;
  }

  // Y crossfades between smooth and nervous random sources. Out1 follows the
  // knob directly; Out2 uses the inverse so the outputs swap their character as
  // Y moves from CCW to CW.
  const int32_t random_interp_out1 = LerpQ12(random_lp_interp, random_hp_interp, frame.coherence_q12);
  const int32_t random_interp_out2 = LerpQ12(random_hp_interp, random_lp_interp, frame.coherence_q12);

  // `room_q12` is the maximum legal deviation from the knob-defined morph
  // position while staying inside the interval [0, 4096]. Using the smaller of
  // the two distances to the endpoints makes the instability collapse naturally
  // at both edges with no extra curve. The Q12 morph domain was kept because it
  // gives exact sine/saw endpoints while staying cheap enough for audio-rate
  // fixed-point math. A 0..4095 domain was considered, but 4096 preserves an
  // exact pure-saw endpoint in the final `>> 12` lerp.
  const uint16_t room_q12 = (frame.morph_q12 < kHalfQ12)
      ? frame.morph_q12
      : uint16_t(kUnityQ12 - frame.morph_q12);

  // `random_interp` is roughly +/-2048, so `>> 11` maps the product back into a
  // legal Q12 offset around the knob-defined morph position. A larger shift
  // would make the center region too tame; a smaller shift would waste corridor
  // resolution by pushing too hard into the clamps.
  const int32_t offset_q12_out1 = (random_interp_out1 * int32_t(room_q12)) >> 11;
  int32_t schrodinger_p_q12_out1 = int32_t(frame.morph_q12) + offset_q12_out1;
  if (schrodinger_p_q12_out1 < 0) schrodinger_p_q12_out1 = 0;
  if (schrodinger_p_q12_out1 > int32_t(kUnityQ12)) schrodinger_p_q12_out1 = int32_t(kUnityQ12);

  const int32_t offset_q12_out2 = (random_interp_out2 * int32_t(room_q12)) >> 11;
  int32_t schrodinger_p_q12_out2 = int32_t(frame.morph_q12) + offset_q12_out2;
  if (schrodinger_p_q12_out2 < 0) schrodinger_p_q12_out2 = 0;
  if (schrodinger_p_q12_out2 > int32_t(kUnityQ12)) schrodinger_p_q12_out2 = int32_t(kUnityQ12);

  out1 = Clamp12(LerpQ12(sine, saw_up, uint16_t(schrodinger_p_q12_out1)));
  out2 = Clamp12(LerpQ12(sine, saw_up_shifted, uint16_t(schrodinger_p_q12_out2)));
}
