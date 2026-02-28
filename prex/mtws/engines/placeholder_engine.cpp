#include "prex/mtws/engines/placeholder_engine.h"

namespace mtws {

PlaceholderEngine::PlaceholderEngine(SineLUT* lut, Kind kind)
    : lut_(lut), kind_(kind), phase_(0), phase_aux_(0), noise_state_(1), noise_lp_(0) {}

void PlaceholderEngine::OnSelected() {
  phase_aux_ = 0;
}

void PlaceholderEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.placeholder.phase_inc = global.pitch_inc;
  frame.placeholder.macro_x = global.macro_x;
  frame.placeholder.macro_y = global.macro_y;
  frame.placeholder.alt = global.mode_alt;
  frame.placeholder.kind = kind_;
}

void PlaceholderEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const PlaceholderControlFrame& p = frame.placeholder;

  uint32_t phase_inc = p.phase_inc;
  if (p.alt) {
    // Alt mode pushes brightness/rate for clearly testable behavior.
    phase_inc += phase_inc >> 2;
  }

  phase_ += phase_inc;
  phase_aux_ += phase_inc * 2U;

  const int32_t s1 = lut_->LookupLinear(phase_);
  const int32_t s2 = lut_->LookupLinear(phase_aux_);

  noise_state_ = noise_state_ * 1664525U + 1013904223U;
  int32_t noise = int32_t((noise_state_ >> 20) & 0x0FFFU) - 2048;

  uint32_t lp_coeff = (uint32_t(p.macro_x) >> 4) + 1U;
  noise_lp_ += int32_t((int64_t(noise - noise_lp_) * lp_coeff) >> 8);

  int32_t out = s1;
  switch (kind_) {
    case VirtualAnalogue:
      out = p.alt ? ((3 * s1 + s2) >> 2) : s1;
      break;

    case WaveShaping:
      if (p.alt) {
        int32_t drive_q12 = 4096 + int32_t(p.macro_x << 1);
        out = Clamp12(int32_t((int64_t(s1) * drive_q12) >> 12));
      } else {
        out = s1;
      }
      break;

    case Formant:
      if (p.alt) {
        int32_t t = int32_t(p.macro_y);
        out = int32_t((int64_t(s1) * (4096 - t) + int64_t(s2) * t) >> 12);
      } else {
        out = (s1 + (s2 >> 1)) >> 1;
      }
      break;

    case FilteredNoise:
      if (p.alt) {
        out = noise_lp_;
      } else {
        out = (s1 + noise_lp_) >> 1;
      }
      break;
  }

  out = Clamp12(out);
  out1 = out;
  out2 = out;
}

}  // namespace mtws
