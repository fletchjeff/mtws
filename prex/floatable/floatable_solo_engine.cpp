#include "prex/floatable/floatable_solo_engine.h"

#include "prex/mtws/wavetables/wavetable_bank_a_64x512.h"
#include "prex/mtws/wavetables/wavetable_bank_b_64x512.h"

FloatableSoloEngine::FloatableSoloEngine(mtws::SineLUT* lut) : lut_(lut), phase_(0) {
  Init();
}

void FloatableSoloEngine::Init() {
  (void)lut_;
  phase_ = 0;
}

void FloatableSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_inc = control.pitch_inc;

  uint32_t x = control.macro_x;
  uint32_t y = control.macro_y;
  if (x > 4095U) x = 4095U;
  if (y > 4095U) y = 4095U;

  uint32_t x0 = x >> 9;
  uint32_t y0 = y >> 9;
  if (x0 > 7U) x0 = 7U;
  if (y0 > 7U) y0 = 7U;
  uint32_t x1 = (x0 + 1U) & 0x7U;

  out.i00 = uint8_t((y0 << 3) + x0);
  out.i10 = uint8_t((y0 << 3) + x1);

  uint32_t x_frac512 = x & 0x1FFU;
  uint32_t x_frac_q12 = x_frac512 << 3;
  if (x_frac512 == 0x1FFU) x_frac_q12 = 4096U;
  out.x_frac_q12 = uint16_t(x_frac_q12);
  out.alt = control.alt;
}

int32_t FloatableSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + ((b - a) * int32_t(t_q12) >> 12);
}

void FloatableSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_inc;

  const int16_t (*table)[512] = frame.alt ? wavetable_bank_b_64x512 : wavetable_bank_a_64x512;

  uint32_t sample_index = phase_ >> 23;
  uint32_t sample_next = (sample_index + 1U) & 0x1FFU;
  uint32_t sample_frac = (phase_ >> 11) & 0x0FFFU;

  int32_t l1 = table[frame.i00][sample_index];
  int32_t l2 = table[frame.i00][sample_next];
  int32_t r1 = table[frame.i10][sample_index];
  int32_t r2 = table[frame.i10][sample_next];

  int32_t s0 = LerpQ12(l1, l2, sample_frac);
  int32_t s1 = LerpQ12(r1, r2, sample_frac);

  int32_t fwd = LerpQ12(s0, s1, frame.x_frac_q12) >> 4;
  int32_t inv = LerpQ12(s1, s0, frame.x_frac_q12) >> 4;

  if (fwd > 2047) fwd = 2047;
  if (fwd < -2048) fwd = -2048;
  if (inv > 2047) inv = 2047;
  if (inv < -2048) inv = -2048;

  out1 = fwd;
  out2 = inv;
}
