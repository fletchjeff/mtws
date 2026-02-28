#include "prex/mtws/engines/wavetable_engine.h"

#include "prex/wavetable/wavetable_bank_a_64x512.h"
#include "prex/wavetable/wavetable_bank_b_64x512.h"

namespace mtws {

namespace {
inline int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  int32_t delta = b - a;
  return a + ((delta * int32_t(t_q12)) >> 12);
}
}  // namespace

WavetableEngine::WavetableEngine() : phase_(0), out1_delay_{0, 0} {}

void WavetableEngine::OnSelected() {
  // Keep phase continuity for smooth transitions.
  out1_delay_[0] = 0;
  out1_delay_[1] = 0;
}

void WavetableEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.wavetable.phase_inc = global.pitch_inc;
  // 32-bit exact mapping of 0..4095 -> 0..(7<<16), with rounding.
  static constexpr uint32_t kGridMaxQ16 = 7U << 16;  // 458752
  uint32_t x_pos_q16 = (uint32_t(global.macro_x) * kGridMaxQ16 + 2047U) / 4095U;
  uint32_t y_pos_q16 = (uint32_t(global.macro_y) * kGridMaxQ16 + 2047U) / 4095U;

  uint32_t x0 = x_pos_q16 >> 16;
  uint32_t y0 = y_pos_q16 >> 16;
  if (x0 > 7U) x0 = 7U;
  if (y0 > 7U) y0 = 7U;
  uint32_t x1 = (x0 < 7U) ? (x0 + 1U) : 7U;
  uint32_t y1 = (y0 < 7U) ? (y0 + 1U) : 7U;

  frame.wavetable.i00 = uint8_t((y0 << 3) + x0);
  frame.wavetable.i10 = uint8_t((y0 << 3) + x1);
  frame.wavetable.i01 = uint8_t((y1 << 3) + x0);
  frame.wavetable.i11 = uint8_t((y1 << 3) + x1);
  frame.wavetable.x_frac_q12 = uint16_t((x_pos_q16 & 0xFFFFU) >> 4);
  frame.wavetable.y_frac_q12 = uint16_t((y_pos_q16 & 0xFFFFU) >> 4);
  frame.wavetable.alt = global.mode_alt;
}

void WavetableEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const WavetableControlFrame& w = frame.wavetable;

  phase_ += w.phase_inc;

  const int16_t (*table)[kSamplesPerWave] = w.alt ? wavetable_bank_b_64x512 : wavetable_bank_a_64x512;
  uint32_t i00 = w.i00;
  uint32_t i10 = w.i10;
  uint32_t i01 = w.i01;
  uint32_t i11 = w.i11;
  uint32_t x_frac = w.x_frac_q12;
  uint32_t y_frac = w.y_frac_q12;

  auto sample_wave = [&](uint32_t wi, uint32_t sample_index, uint32_t sample_next, uint32_t sample_frac) -> int32_t {
    int32_t s1 = table[wi][sample_index];
    int32_t s2 = table[wi][sample_next];
    return LerpQ12(s1, s2, sample_frac);
  };

  auto bilinear_from_phase = [&](uint32_t phase) -> int32_t {
    // 512-sample wave: top 9 bits are integer index, next 12 bits are fraction.
    uint32_t sample_index = phase >> 23;
    uint32_t sample_next = (sample_index + 1U) & 0x1FFU;
    uint32_t sample_frac = (phase >> 11) & 0x0FFFU;

    int32_t s00 = sample_wave(i00, sample_index, sample_next, sample_frac);
    int32_t s10 = sample_wave(i10, sample_index, sample_next, sample_frac);
    int32_t s01 = sample_wave(i01, sample_index, sample_next, sample_frac);
    int32_t s11 = sample_wave(i11, sample_index, sample_next, sample_frac);

    int32_t top = LerpQ12(s00, s10, x_frac);
    int32_t bot = LerpQ12(s01, s11, x_frac);
    return LerpQ12(top, bot, y_frac);
  };

  int32_t fwd = bilinear_from_phase(phase_) >> 4;
  if (fwd > 2047) fwd = 2047;
  if (fwd < -2048) fwd = -2048;

  // Use a small delayed inverted tap for out2 to avoid exact mono cancellation.
  int32_t delayed = out1_delay_[1];
  out1_delay_[1] = out1_delay_[0];
  out1_delay_[0] = fwd;

  int32_t inv = -delayed;
  if (inv > 2047) inv = 2047;
  if (inv < -2048) inv = -2048;

  out1 = fwd;
  out2 = inv;
}

}  // namespace mtws
