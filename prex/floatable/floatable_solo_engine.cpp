#include "prex/floatable/floatable_solo_engine.h"

#include "prex/mtws/wavetables/floatable_bank_1_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_2_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_3_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_4_16x256.h"

namespace {
constexpr uint32_t kNumSourceWaves = 16U;
constexpr uint32_t kInterpolationCells = kNumSourceWaves - 1U;
constexpr uint32_t kRenderedTableSize = 256U;
constexpr uint32_t kRenderedTableMask = kRenderedTableSize - 1U;
constexpr uint32_t kRenderedTableIndexShift = 24U;
constexpr uint32_t kRenderedTableFracShift = 12U;

// Clamps a rendered wavetable sample back into the signed 16-bit source domain.
static inline int16_t ClampToInt16(int32_t sample) {
  if (sample > 32767) return 32767;
  if (sample < -32768) return -32768;
  return int16_t(sample);
}

// Clamps the rendered output sample to the signed 12-bit ComputerCard domain.
static inline int32_t ClampAudio12(int32_t sample) {
  if (sample > 2047) return 2047;
  if (sample < -2048) return -2048;
  return sample;
}
}  // namespace

// Initializes dependency and runtime phase state.
FloatableSoloEngine::FloatableSoloEngine(mtws::SineLUT* lut)
    : lut_(lut), phase_(0) {
  Init();
}

// Resets oscillator phase accumulator.
void FloatableSoloEngine::Init() {
  (void)lut_;
  phase_ = 0;
}

// Builds both rendered output tables at control rate from the shared curated
// 16x256 bank set. Normal mode routes banks 1/2 to Out1/Out2 and alt mode
// routes banks 3/4 to Out1/Out2, which is the same mapping used in `mtws`.
void FloatableSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_inc = control.pitch_inc;

  auto render_wavetable_from_axis = [&](const int16_t source_waves[16][256],
                                        uint32_t axis_code,
                                        int16_t rendered_wave[256]) {
    if (axis_code > 4095U) axis_code = 4095U;

    // Map 0..4095 across 15 interpolation cells so the full sweep moves from
    // wave 0 to wave 15. The endpoint special-case preserves exact access to
    // the final wave instead of stopping one fraction short.
    uint32_t wave_pos_q12 = axis_code * kInterpolationCells;
    uint32_t wave_a = wave_pos_q12 >> 12;
    if (wave_a > (kNumSourceWaves - 2U)) wave_a = kNumSourceWaves - 2U;
    uint32_t wave_b = wave_a + 1U;
    uint32_t wave_frac_q12 = wave_pos_q12 & 0x0FFFU;
    if (axis_code == 4095U) wave_frac_q12 = 4096U;

    for (uint32_t sample_index = 0; sample_index < kRenderedTableSize; ++sample_index) {
      int32_t sample_a = source_waves[wave_a][sample_index];
      int32_t sample_b = source_waves[wave_b][sample_index];
      rendered_wave[sample_index] = ClampToInt16(LerpQ12(sample_a, sample_b, wave_frac_q12));
    }
  };

  const int16_t (*out1_source)[256] =
      control.alt ? floatable_bank_3_16x256 : floatable_bank_1_16x256;
  const int16_t (*out2_source)[256] =
      control.alt ? floatable_bank_4_16x256 : floatable_bank_2_16x256;

  render_wavetable_from_axis(out1_source, control.macro_x, out.rendered_out1);
  render_wavetable_from_axis(out2_source, control.macro_y, out.rendered_out2);
}

// Interpolates between a and b using Q12 fraction t_q12.
int32_t FloatableSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + ((b - a) * int32_t(t_q12) >> 12);
}

// Renders one sample by phase-interpolating the two finished 256-sample tables
// built by the control core. Both outputs share the same phase accumulator so
// pitch stays locked while X and Y drive independent timbral motion.
void FloatableSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_inc;

  // 256-sample table: top 8 phase bits select sample, next 12 bits are used as
  // the interpolation fraction. This keeps the read path cheap while matching
  // the reduced table length used to stay within the working frame size.
  uint32_t sample_index = phase_ >> kRenderedTableIndexShift;
  uint32_t sample_next = (sample_index + 1U) & kRenderedTableMask;
  uint32_t sample_frac = (phase_ >> kRenderedTableFracShift) & 0x0FFFU;

  auto render_output = [&](const int16_t wave[256]) -> int32_t {
    int32_t sample_a = wave[sample_index];
    int32_t sample_b = wave[sample_next];
    return ClampAudio12(LerpQ12(sample_a, sample_b, sample_frac) >> 4);
  };

  out1 = render_output(frame.rendered_out1);
  out2 = render_output(frame.rendered_out2);
}
