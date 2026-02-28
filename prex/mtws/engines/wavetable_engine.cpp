#include "prex/mtws/engines/wavetable_engine.h"

#include "prex/wavetable/wavetable_data.h"

namespace mtws {

WavetableEngine::WavetableEngine() : phase_(0) {}

void WavetableEngine::OnSelected() {
  // Keep phase continuity for smooth transitions.
}

void WavetableEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  frame.wavetable.phase_inc = global.pitch_inc;

  uint32_t wave_pos_q16 = uint32_t((uint64_t(global.macro_x) * 59U * 65536U + 2047U) / 4095U);
  if (global.mode_alt) {
    uint32_t max_pos_q16 = 59U << 16;
    if (wave_pos_q16 <= max_pos_q16) {
      wave_pos_q16 = max_pos_q16 - wave_pos_q16;
    } else {
      wave_pos_q16 = 0;
    }
  }

  frame.wavetable.wave_pos_q16 = wave_pos_q16;
  frame.wavetable.macro_y = global.macro_y;
  frame.wavetable.alt = global.mode_alt;
}

void WavetableEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const WavetableControlFrame& w = frame.wavetable;

  uint32_t phase_inc = w.phase_inc;
  if (w.alt) {
    phase_inc += uint32_t((uint64_t(phase_inc) * w.macro_y) >> 13);  // up to +50%
  }
  phase_ += phase_inc;

  uint32_t wave_index_1 = w.wave_pos_q16 >> 16;
  if (wave_index_1 >= kNumWaveforms) wave_index_1 = kNumWaveforms - 1;
  uint32_t wave_frac = w.wave_pos_q16 & 0xFFFFU;
  uint32_t wave_index_2 = (wave_index_1 + 1U) % kNumWaveforms;

  uint64_t pos = uint64_t(phase_) * kSamplesPerWave;
  uint32_t sample_index = uint32_t(pos >> 32);
  if (sample_index >= kSamplesPerWave) sample_index = kSamplesPerWave - 1;
  uint32_t sample_frac = uint32_t((pos >> 16) & 0xFFFFU);
  uint32_t sample_next = (sample_index + 1U) % kSamplesPerWave;

  int32_t w1s1 = wavetable[wave_index_1][sample_index];
  int32_t w1s2 = wavetable[wave_index_1][sample_next];
  int32_t wave1_sample = int32_t((int64_t(w1s2) * sample_frac + int64_t(w1s1) * (65536 - sample_frac)) >> 16);

  int32_t w2s1 = wavetable[wave_index_2][sample_index];
  int32_t w2s2 = wavetable[wave_index_2][sample_next];
  int32_t wave2_sample = int32_t((int64_t(w2s2) * sample_frac + int64_t(w2s1) * (65536 - sample_frac)) >> 16);

  int32_t mixed = int32_t((int64_t(wave2_sample) * wave_frac + int64_t(wave1_sample) * (65536 - wave_frac)) >> 16);
  int32_t out = mixed >> 4;

  if (out > 2047) out = 2047;
  if (out < -2048) out = -2048;

  out1 = out;
  out2 = out;
}

}  // namespace mtws
