#include "prex/mtws/engines/wavetable_engine.h"

#include "prex/mtws/wavetables/wavetable_bank_a_64x512.h"
#include "prex/mtws/wavetables/wavetable_bank_b_64x512.h"

namespace mtws {

namespace {
inline int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  int32_t delta = b - a;
  return a + ((delta * int32_t(t_q12)) >> 12);
}
}  // namespace

WavetableEngine::WavetableEngine()
    : phase_(0),
      out2_pair_valid_(false),
      out2_active_l_(0),
      out2_active_r_(0),
      out2_pending_valid_(false),
      out2_pending_l_(0),
      out2_pending_r_(0),
      out2_was_in_fade_window_(false) {}

void WavetableEngine::OnSelected() {
  // Keep phase continuity for smooth transitions.
  out2_pair_valid_ = false;
  out2_pending_valid_ = false;
  out2_was_in_fade_window_ = false;
}

void WavetableEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.wavetable.phase_inc = global.pitch_inc;

  // Map 0..4095 to an 8x8 grid with pure shifts:
  // cell = knob>>9 (0..7), intra-cell frac = knob&0x1ff (0..511).
  uint32_t x_code = global.macro_x;
  uint32_t y_code = global.macro_y;
  if (x_code > 4095U) x_code = 4095U;
  if (y_code > 4095U) y_code = 4095U;

  uint32_t x0 = x_code >> 9;
  uint32_t y0 = y_code >> 9;
  if (x0 > 7U) x0 = 7U;
  if (y0 > 7U) y0 = 7U;
  uint32_t x1 = (x0 + 1U) & 0x7U;  // wrap 7->0 so last segment still interpolates

  frame.wavetable.i00 = uint8_t((y0 << 3) + x0);
  frame.wavetable.i10 = uint8_t((y0 << 3) + x1);
  frame.wavetable.i01 = frame.wavetable.i00;  // unused in X-only morph mode
  frame.wavetable.i11 = frame.wavetable.i10;  // unused in X-only morph mode
  uint32_t x_frac512 = x_code & 0x1FFU;
  uint32_t x_frac_q12 = x_frac512 << 3;  // 0..4088
  if (x_frac512 == 0x1FFU) x_frac_q12 = 4096U;  // exact endpoint at cell boundary
  frame.wavetable.x_frac_q12 = uint16_t(x_frac_q12);
  frame.wavetable.y_frac_q12 = 0;
  frame.wavetable.aa_level = 0;
  frame.wavetable.alt = global.mode_alt;
}

void WavetableEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const WavetableControlFrame& w = frame.wavetable;

  phase_ += w.phase_inc;

  const int16_t (*table)[kSamplesPerWave] = w.alt ? wavetable_bank_b_64x512 : wavetable_bank_a_64x512;
  uint32_t i00 = w.i00;
  uint32_t i10 = w.i10;
  uint32_t x_frac = w.x_frac_q12;

  auto sample_wave = [&](uint32_t wi, uint32_t sample_index, uint32_t sample_next, uint32_t sample_frac) -> int32_t {
    int32_t s1 = table[wi][sample_index];
    int32_t s2 = table[wi][sample_next];
    return LerpQ12(s1, s2, sample_frac);
  };

  auto x_morph_pair_from_phase = [&](uint8_t left, uint8_t right, uint32_t phase) -> int32_t {
    // 512-sample wave: top 9 bits are integer index, next 12 bits are fraction.
    uint32_t sample_index = phase >> 23;
    uint32_t sample_next = (sample_index + 1U) & 0x1FFU;
    uint32_t sample_frac = (phase >> 11) & 0x0FFFU;

    int32_t s0 = sample_wave(left, sample_index, sample_next, sample_frac);
    int32_t s1 = sample_wave(right, sample_index, sample_next, sample_frac);
    int32_t fwd = LerpQ12(s0, s1, x_frac);
    uint32_t x_pingpong = (x_frac <= 2048U) ? ((2048U - x_frac) << 1) : ((x_frac - 2048U) << 1);
    if (x_pingpong > 4096U) x_pingpong = 4096U;
    int32_t inv = LerpQ12(s0, s1, x_pingpong);
    return (fwd << 16) | (inv & 0xFFFF);
  };

  uint8_t new_l = uint8_t(i00);
  uint8_t new_r = uint8_t(i10);
  if (!out2_pair_valid_) {
    out2_pair_valid_ = true;
    out2_active_l_ = new_l;
    out2_active_r_ = new_r;
  }
  if (new_l != out2_active_l_ || new_r != out2_active_r_) {
    out2_pending_valid_ = true;
    out2_pending_l_ = new_l;
    out2_pending_r_ = new_r;
  }

  int32_t pair = x_morph_pair_from_phase(uint8_t(i00), uint8_t(i10), phase_);
  int32_t fwd = (pair >> 16) >> 4;
  if (fwd > 2047) fwd = 2047;
  if (fwd < -2048) fwd = -2048;

  int32_t active_pair = x_morph_pair_from_phase(out2_active_l_, out2_active_r_, phase_);
  int32_t inv = int16_t(active_pair & 0xFFFF) >> 4;
  bool in_fade_window = (phase_ >= kOut2FadeStartPhase);
  if (out2_pending_valid_ && in_fade_window) {
    int32_t pending_pair = x_morph_pair_from_phase(out2_pending_l_, out2_pending_r_, phase_);
    int32_t inv_pending = int16_t(pending_pair & 0xFFFF) >> 4;

    uint32_t fade_t_q12 = (phase_ - kOut2FadeStartPhase) >> 18;  // quarter-cycle -> 0..4095
    if (fade_t_q12 > 4096U) fade_t_q12 = 4096U;
    inv = LerpQ12(inv, inv_pending, fade_t_q12);
  }

  if (out2_pending_valid_ && out2_was_in_fade_window_ && !in_fade_window) {
    out2_active_l_ = out2_pending_l_;
    out2_active_r_ = out2_pending_r_;
    out2_pending_valid_ = false;
  }
  out2_was_in_fade_window_ = in_fade_window;

  if (inv > 2047) inv = 2047;
  if (inv < -2048) inv = -2048;

  out1 = fwd;
  out2 = inv;
}

}  // namespace mtws
