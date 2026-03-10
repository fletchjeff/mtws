#include "prex/mtws/engines/floatable_engine.h"

#include "prex/mtws/wavetables/floatable_bank_1_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_2_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_3_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_4_16x256.h"

namespace mtws {

namespace {
constexpr uint32_t kNumSourceWaves = 16U;
constexpr uint32_t kInterpolationCells = kNumSourceWaves - 1U;
constexpr uint32_t kRenderedTableSize = 256U;

// Interpolates between `a` and `b` using a Q12 fraction. `4096` is used
// instead of `4095` so the endpoint can reach the last stored sample exactly.
inline int32_t LerpQ12Local(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + ((b - a) * int32_t(t_q12) >> 12);
}

// Clamps the rendered source-domain sample before it is stored into the control
// frame. The curated bank headers already contain int16_t values, but the lerp
// runs in int32_t so the final write is explicit and safe.
inline int16_t ClampToInt16(int32_t sample) {
  if (sample > 32767) return 32767;
  if (sample < -32768) return -32768;
  return int16_t(sample);
}

// Clamps the audio sample to the signed 12-bit ComputerCard output range.
inline int32_t ClampAudio12(int32_t sample) {
  if (sample > 2047) return 2047;
  if (sample < -2048) return -2048;
  return sample;
}

// Builds one finished 256-sample wavetable from a 16-wave source bank.
// `axis_code` is the knob-domain 0..4095 value. It is mapped across 15
// interpolation cells so `0` selects wave 0 and `4095` reaches wave 15 exactly.
void RenderBankFromAxis(const int16_t source_waves[kNumSourceWaves][kRenderedTableSize],
                        uint32_t axis_code,
                        int16_t rendered_wave[kRenderedTableSize]) {
  if (axis_code > 4095U) axis_code = 4095U;

  uint32_t wave_pos_q12 = axis_code * kInterpolationCells;
  uint32_t wave_a = wave_pos_q12 >> 12;
  if (wave_a > (kNumSourceWaves - 2U)) wave_a = kNumSourceWaves - 2U;
  uint32_t wave_b = wave_a + 1U;
  uint32_t wave_frac_q12 = wave_pos_q12 & 0x0FFFU;
  if (axis_code == 4095U) wave_frac_q12 = 4096U;

  for (uint32_t sample_index = 0; sample_index < kRenderedTableSize; ++sample_index) {
    int32_t sample_a = source_waves[wave_a][sample_index];
    int32_t sample_b = source_waves[wave_b][sample_index];
    rendered_wave[sample_index] = ClampToInt16(LerpQ12Local(sample_a, sample_b, wave_frac_q12));
  }
}
}  // namespace

// Initializes the oscillator with a cleared shared phase accumulator and blank
// cached rendered tables used by the alternating control-rate renderer.
FloatableEngine::FloatableEngine()
    : phase_(0), caches_primed_(false), render_out1_on_next_tick_(true) {
  for (uint32_t sample_index = 0; sample_index < kRenderedTableSize; ++sample_index) {
    cached_rendered_out1_[sample_index] = 0;
    cached_rendered_out2_[sample_index] = 0;
  }
}

void FloatableEngine::OnSelected() {
  // Keep phase continuity for smooth slot changes.
}

// Copies pitch state and updates one rendered table per control tick. Out1 and
// Out2 table renders are alternated so each axis is recomputed every other
// control frame, which reduces core-1 interpolation workload. Both cached
// tables are then copied into the published control frame so the audio core
// always receives valid Out1 and Out2 tables.
void FloatableEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.floatable.phase_inc = global.pitch_inc;

  const int16_t (*out1_source)[kRenderedTableSize] =
      global.mode_alt ? floatable_bank_3_16x256 : floatable_bank_1_16x256;
  const int16_t (*out2_source)[kRenderedTableSize] =
      global.mode_alt ? floatable_bank_4_16x256 : floatable_bank_2_16x256;

  if (!caches_primed_) {
    RenderBankFromAxis(out1_source, global.macro_x, cached_rendered_out1_);
    RenderBankFromAxis(out2_source, global.macro_y, cached_rendered_out2_);
    caches_primed_ = true;
    render_out1_on_next_tick_ = true;
  } else if (render_out1_on_next_tick_) {
    RenderBankFromAxis(out1_source, global.macro_x, cached_rendered_out1_);
    render_out1_on_next_tick_ = false;
  } else {
    RenderBankFromAxis(out2_source, global.macro_y, cached_rendered_out2_);
    render_out1_on_next_tick_ = true;
  }

  for (uint32_t sample_index = 0; sample_index < kRenderedTableSize; ++sample_index) {
    frame.floatable.rendered_out1[sample_index] = cached_rendered_out1_[sample_index];
    frame.floatable.rendered_out2[sample_index] = cached_rendered_out2_[sample_index];
  }
}

// Interpolates between `a` and `b` using a Q12 fraction. `4096` is used instead
// of `4095` so the endpoint can reach the last stored sample exactly.
int32_t FloatableEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + ((b - a) * int32_t(t_q12) >> 12);
}

// Reads the two rendered tables at audio rate. The top 8 phase bits select the
// integer sample in the 256-point table and the next 12 bits provide the Q12
// phase interpolation fraction. This reduces audio-rate work compared with the
// old 64x512 direct-read path at the cost of storing two rendered tables per
// double-buffered control frame.
void FloatableEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const FloatableControlFrame& w = frame.floatable;

  phase_ += w.phase_inc;

  uint32_t sample_index = phase_ >> kRenderedTableIndexShift;
  uint32_t sample_next = (sample_index + 1U) & kRenderedTableMask;
  uint32_t sample_frac = (phase_ >> kRenderedTableFracShift) & 0x0FFFU;

  auto render_output = [&](const int16_t wave[kRenderedTableSize]) -> int32_t {
    int32_t sample_a = wave[sample_index];
    int32_t sample_b = wave[sample_next];
    return ClampAudio12(LerpQ12(sample_a, sample_b, sample_frac) >> 4);
  };

  out1 = render_output(w.rendered_out1);
  out2 = render_output(w.rendered_out2);
}

}  // namespace mtws
