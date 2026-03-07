#include "prex/bender/bender_solo_engine.h"

namespace {
// Converts knob domain (0..4095) to a Q12 interpolation domain (0..4096).
// The extra top code point allows exact unity interpolation at full-scale knob.
inline uint32_t ToQ12(uint16_t v) { return (v >= 4095U) ? 4096U : uint32_t(v); }

// Maps macro X into the Utility-Pair gain domain used before the folder.
// `128` represents unity input gain because the folded input is scaled by `>> 7`.
// The upper bound `2048` was chosen because it matches the original Utility-Pair
// maximum multiplier range (~16x), giving a familiar amount of folding.
// Alternative considered: stop at `1024` (~8x) for a gentler maximum drive.
// CPU vs memory tradeoff: this is one multiply/shift at control rate and avoids
// storing a separate drive LUT for a simple linear mapping.
inline uint16_t FoldDriveQ7FromMacroX(uint16_t macro_x) {
  uint32_t x_q12 = ToQ12(macro_x);
  return uint16_t(128U + ((x_q12 * 1920U + 2048U) >> 12));
}

// Maps macro Y into the Utility-Pair crush step domain using the same squared
// response as the reference implementation. `0` means "effectively off" and is
// clamped to `1` inside CrushFunction, while `4095` gives the coarsest quantizer.
// Alternative considered: linear `1..4095`, but the squared curve gives more
// usable low-end travel before the crusher becomes extreme.
// CPU vs memory tradeoff: one control-rate multiply/shift keeps the mapping
// cheap and avoids an extra lookup table.
inline int16_t CrushAmountFromMacroY(uint16_t macro_y) {
  return int16_t((uint32_t(macro_y) * uint32_t(macro_y)) >> 12);
}
}  // namespace

// Stores LUT dependency and initializes oscillator and fold-state history.
BenderSoloEngine::BenderSoloEngine(mtws::SineLUT* lut)
    : lut_(lut),
      phase_(0),
      last_fold_integral_pre_(0),
      last_fold_input_pre_(0),
      last_fold_integral_post_(0),
      last_fold_input_post_(0),
      last_alt_(false) {}

// Resets oscillator phase and antialiased folder history to deterministic startup state.
void BenderSoloEngine::Init() {
  phase_ = 0;
  last_fold_integral_pre_ = 0;
  last_fold_input_pre_ = 0;
  last_fold_integral_post_ = 0;
  last_fold_input_post_ = 0;
  last_alt_ = false;
}

// Copies control-frame values into a lightweight RenderFrame for audio-rate use.
// `Main` becomes oscillator pitch, `X` becomes fold drive, and `Y` becomes the
// Utility-Pair-style crush amount for Out2.
void BenderSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_increment = control.pitch_inc;
  out.fold_drive_q7 = FoldDriveQ7FromMacroX(control.macro_x);
  out.fold_mix_q12 = uint16_t(ToQ12(control.macro_x));
  out.crush_amount = CrushAmountFromMacroY(control.macro_y);
  out.alt = control.alt;
}

// Clamps to board audio range to avoid wrap/overflow at the DAC boundary.
int32_t BenderSoloEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

// Utility-Pair fold transfer function. Values within +/-2048 pass through,
// while larger magnitudes are reflected every 4096 samples.
int32_t BenderSoloEngine::FoldFunction(int32_t x) {
  int32_t period = 8192;
  x = ((x + 2048) % period + period) % period;

  if (x < 4096) {
    return x - 2048;
  }
  return (8191 - x) - 2048;
}

// Antiderivative of FoldFunction used for the derivative antialiasing step.
// This follows the Utility-Pair implementation directly so the folded sine uses
// the same transfer curve and the same antialiasing strategy.
int32_t BenderSoloEngine::FoldIntegral(int32_t x) {
  int32_t period = 8192;
  x = ((x + 2048) % period + period) % period;
  int32_t x2 = x * 2;
  if (x < 4096) {
    return ((x2 + 1) * (x2 - 8191)) >> 3;
  }
  return -((x2 - 8191) * (x2 - 16383)) >> 3;
}

// Differentiates successive fold integrals to suppress aliasing at fold corners.
// If the input sample did not move, the exact fold function is cheaper and
// produces the same result.
int32_t BenderSoloEngine::AntiAliasedFold(int32_t x, int32_t& last_integral, int32_t& last_input) {
  int32_t ret = 0;
  if (x == last_input) {
    ret = FoldFunction(x);
  } else {
    int32_t integral = FoldIntegral(x);
    ret = (integral - last_integral) / (x - last_input);
    last_input = x;
    last_integral = integral;
  }
  return Clamp12(ret);
}

// Utility-Pair bitcrusher transfer. `amount` is the quantizer step size in the
// signed 12-bit sample domain, so larger values produce fewer output levels.
int32_t BenderSoloEngine::CrushFunction(int32_t x, int32_t amount) {
  if (amount < 1) amount = 1;
  x += -2047 + (amount >> 1);
  x = x - (((x % amount) + amount) % amount) + 2047;
  if (x < -2047) x = -2047;
  if (x > 2047) x = 2047;
  return x;
}

// Linear interpolation where `mix_q12 = 0` returns the dry signal and
// `mix_q12 = 4096` returns the wet signal.
int32_t BenderSoloEngine::LerpQ12(int32_t dry, int32_t wet, uint32_t mix_q12) {
  if (mix_q12 > 4096U) mix_q12 = 4096U;
  return dry + int32_t((int64_t(wet - dry) * int32_t(mix_q12)) >> 12);
}

// Renders one sine sample, applies fold drive for Out1 and crush amount for
// Out2, then sends the same oscillator through the selected Utility-Pair
// processing order. Normal mode is the current A/B comparison patch; alt mode
// evaluates "fold then crush" versus "crush then fold".
void BenderSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  if (frame.alt != last_alt_) {
    last_fold_integral_pre_ = 0;
    last_fold_input_pre_ = 0;
    last_fold_integral_post_ = 0;
    last_fold_input_post_ = 0;
    last_alt_ = frame.alt;
  }

  phase_ += frame.phase_increment;

  int32_t sine = lut_->LookupLinear(phase_);
  int32_t folded_input = int32_t((int64_t(sine) * frame.fold_drive_q7) >> 7);
  int32_t folded = AntiAliasedFold(folded_input, last_fold_integral_pre_, last_fold_input_pre_);
  int32_t crushed = CrushFunction(sine, frame.crush_amount);

  if (!frame.alt) {
    out1 = Clamp12(LerpQ12(sine, folded, frame.fold_mix_q12));
    out2 = crushed;
    return;
  }

  int32_t fold_then_crush = CrushFunction(folded, frame.crush_amount);
  int32_t crush_then_fold_input = int32_t((int64_t(crushed) * frame.fold_drive_q7) >> 7);
  int32_t crush_then_fold = AntiAliasedFold(crush_then_fold_input, last_fold_integral_post_, last_fold_input_post_);

  out1 = fold_then_crush;
  out2 = crush_then_fold;
}
