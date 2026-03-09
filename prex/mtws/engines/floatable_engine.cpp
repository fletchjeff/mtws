#include "prex/mtws/engines/floatable_engine.h"

#include "prex/mtws/wavetables/wavetable_bank_a_64x512.h"
#include "prex/mtws/wavetables/wavetable_bank_b_64x512.h"

namespace mtws {

namespace {
// Interpolates between two signed samples with a Q12 fraction where
// `t_q12 = 0` returns `a` and `t_q12 = 4096` returns `b`.
inline int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  int32_t delta = b - a;
  return a + ((delta * int32_t(t_q12)) >> 12);
}

// Clamps the rendered sample to the signed 12-bit ComputerCard audio domain.
inline int32_t ClampAudio12(int32_t sample) {
  if (sample > 2047) return 2047;
  if (sample < -2048) return -2048;
  return sample;
}
}  // namespace

// Initializes the oscillator with a cleared phase accumulator.
FloatableEngine::FloatableEngine() : phase_(0) {}

void FloatableEngine::OnSelected() {
  // Keep phase continuity for smooth slot changes.
}

// Converts X/Y controls into the four surrounding wavetable indices plus their
// in-cell interpolation fractions. Inputs are knob-domain 0..4095 and outputs
// are an 8x8 non-wrapping grid with Q12 fractions for audio-rate morphing.
void FloatableEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.floatable.phase_inc = global.pitch_inc;

  // Map 0..4095 onto 7 interpolation cells in Q12. This gives a non-wrapping
  // 8x8 surface: 0 selects the first row/column, 4095 reaches the last one.
  uint32_t x_code = global.macro_x;
  uint32_t y_code = global.macro_y;
  if (x_code > 4095U) x_code = 4095U;
  if (y_code > 4095U) y_code = 4095U;

  uint32_t x_pos_q12 = x_code * 7U;
  uint32_t y_pos_q12 = y_code * 7U;
  uint32_t x0 = x_pos_q12 >> 12;
  uint32_t y0 = y_pos_q12 >> 12;
  if (x0 > 6U) x0 = 6U;
  if (y0 > 6U) y0 = 6U;
  uint32_t x1 = x0 + 1U;
  uint32_t y1 = y0 + 1U;

  frame.floatable.i00 = uint8_t((y0 << 3) + x0);
  frame.floatable.i10 = uint8_t((y0 << 3) + x1);
  frame.floatable.i01 = uint8_t((y1 << 3) + x0);
  frame.floatable.i11 = uint8_t((y1 << 3) + x1);

  uint32_t x_frac_q12 = x_pos_q12 & 0x0FFFU;
  uint32_t y_frac_q12 = y_pos_q12 & 0x0FFFU;
  if (x_code == 4095U) x_frac_q12 = 4096U;
  if (y_code == 4095U) y_frac_q12 = 4096U;
  frame.floatable.x_frac_q12 = uint16_t(x_frac_q12);
  frame.floatable.y_frac_q12 = uint16_t(y_frac_q12);
  frame.floatable.aa_level = 0;
  frame.floatable.alt = global.mode_alt;
}

// Renders one stereo sample from the current 8x8 wavetable surface. Out1 uses
// the forward X morph, while Out2 reuses the same corner samples for the
// inverse X morph across the same Y position.
void FloatableEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const FloatableControlFrame& w = frame.floatable;

  phase_ += w.phase_inc;

  const int16_t (*table)[kSamplesPerWave] = w.alt ? wavetable_bank_b_64x512 : wavetable_bank_a_64x512;
  uint32_t x_frac = w.x_frac_q12;
  uint32_t y_frac = w.y_frac_q12;

  auto sample_wave = [&](uint32_t wi, uint32_t sample_index, uint32_t sample_next, uint32_t sample_frac) -> int32_t {
    int32_t s1 = table[wi][sample_index];
    int32_t s2 = table[wi][sample_next];
    return LerpQ12(s1, s2, sample_frac);
  };

  // 512-sample wave: top 9 bits are integer index, next 12 bits are fraction.
  uint32_t sample_index = phase_ >> 23;
  uint32_t sample_next = (sample_index + 1U) & 0x1FFU;
  uint32_t sample_frac = (phase_ >> 11) & 0x0FFFU;

  int32_t s00 = sample_wave(w.i00, sample_index, sample_next, sample_frac);
  int32_t s10 = sample_wave(w.i10, sample_index, sample_next, sample_frac);
  int32_t s01 = sample_wave(w.i01, sample_index, sample_next, sample_frac);
  int32_t s11 = sample_wave(w.i11, sample_index, sample_next, sample_frac);

  int32_t top = LerpQ12(s00, s10, x_frac);
  int32_t bottom = LerpQ12(s01, s11, x_frac);
  int32_t fwd = LerpQ12(top, bottom, y_frac) >> 4;

  // The inverse-X output reuses the same corners instead of fetching and
  // phase-interpolating the four waves a second time.
  int32_t top_inv = s00 + s10 - top;
  int32_t bottom_inv = s01 + s11 - bottom;
  int32_t inv = LerpQ12(top_inv, bottom_inv, y_frac) >> 4;

  out1 = ClampAudio12(fwd);
  out2 = ClampAudio12(inv);
}

}  // namespace mtws
