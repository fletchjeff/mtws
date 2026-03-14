#include "prex/floatable/floatable_solo_engine.h"

#include "prex/mtws/wavetables/floatable_bank_1_16x256.h"
#include "prex/mtws/wavetables/floatable_bank_4_16x256.h"

namespace {
constexpr uint32_t kNumSourceWaves = 16U;
constexpr uint32_t kInterpolationCells = kNumSourceWaves - 1U;
constexpr uint32_t kSourceTableSize = 256U;
constexpr uint32_t kSourceTableMask = kSourceTableSize - 1U;
constexpr uint32_t kSourceTableIndexShift = 24U;
constexpr uint32_t kSourceTableFracShift = 12U;

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

// Converts the solo control frame into the compact morph state used by the
// integrated engine. X and Y pick independent positions inside the same active
// curated bank, while alt switches between the normal and alternate bank.
void FloatableSoloEngine::BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const {
  out.phase_inc = control.pitch_inc;
  out.use_alt_banks = control.alt ? 1U : 0U;
  ComputeWaveSelection(control.macro_x, out.out1_wave_index, out.out1_wave_frac_q12);
  ComputeWaveSelection(control.macro_y, out.out2_wave_index, out.out2_wave_frac_q12);
}

// Interpolates between a and b using Q12 fraction t_q12.
int32_t FloatableSoloEngine::LerpQ12(int32_t a, int32_t b, uint32_t t_q12) {
  if (t_q12 > 4096U) t_q12 = 4096U;
  return a + ((b - a) * int32_t(t_q12) >> 12);
}

// Converts one axis code into a source-wave selection pair with exact endpoint
// handling for the final stored source wave.
void FloatableSoloEngine::ComputeWaveSelection(uint32_t axis_code, uint8_t& wave_index, uint16_t& wave_frac_q12) {
  if (axis_code > 4095U) axis_code = 4095U;

  uint32_t wave_pos_q12 = axis_code * kInterpolationCells;
  uint32_t index = wave_pos_q12 >> 12;
  if (index > (kNumSourceWaves - 2U)) index = kNumSourceWaves - 2U;

  uint32_t frac_q12 = wave_pos_q12 & 0x0FFFU;
  if (axis_code == 4095U) frac_q12 = 4096U;

  wave_index = static_cast<uint8_t>(index);
  wave_frac_q12 = static_cast<uint16_t>(frac_q12);
}

// Reads one sample from the current source bank by interpolating in phase
// first, then across adjacent source waves selected by the control frame.
int32_t FloatableSoloEngine::RenderMorphedBankSample(const int16_t (*source_waves)[256],
                                                     uint32_t wave_index,
                                                     uint32_t wave_frac_q12,
                                                     uint32_t sample_index,
                                                     uint32_t sample_next,
                                                     uint32_t sample_frac_q12) {
  if (wave_index > (kNumSourceWaves - 2U)) wave_index = kNumSourceWaves - 2U;

  const int16_t* wave_a = source_waves[wave_index];
  const int16_t* wave_b = source_waves[wave_index + 1U];

  int32_t sample_a = LerpQ12(wave_a[sample_index], wave_a[sample_next], sample_frac_q12);
  int32_t sample_b = LerpQ12(wave_b[sample_index], wave_b[sample_next], sample_frac_q12);
  int32_t morphed_sample = LerpQ12(sample_a, sample_b, wave_frac_q12);
  return ClampAudio12(morphed_sample >> 4);
}

// Renders one sample directly from the active source bank. Both outputs share
// the same oscillator phase so pitch stays locked while X and Y choose
// different morph positions inside the same timbral bank.
void FloatableSoloEngine::RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2) {
  phase_ += frame.phase_inc;

  uint32_t sample_index = phase_ >> kSourceTableIndexShift;
  uint32_t sample_next = (sample_index + 1U) & kSourceTableMask;
  uint32_t sample_frac_q12 = (phase_ >> kSourceTableFracShift) & 0x0FFFU;

  const int16_t (*source)[256] =
      (frame.use_alt_banks != 0U) ? floatable_bank_4_16x256 : floatable_bank_1_16x256;

  out1 = RenderMorphedBankSample(source,
                                 frame.out1_wave_index,
                                 frame.out1_wave_frac_q12,
                                 sample_index,
                                 sample_next,
                                 sample_frac_q12);
  out2 = RenderMorphedBankSample(source,
                                 frame.out2_wave_index,
                                 frame.out2_wave_frac_q12,
                                 sample_index,
                                 sample_next,
                                 sample_frac_q12);
}
