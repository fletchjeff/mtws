#include "prex/mtws/engines/sawsome_engine.h"

namespace mtws {

namespace {
constexpr int32_t kUnityQ12 = 4096;
constexpr int32_t kDetuneOffsetQ16[kNumSawsomeVoices] = {
    -3932, -2621, -1311, 0, 1311, 2621, 3932,
};
constexpr int32_t kPanQ12[kNumSawsomeVoices] = {
    -3686, -2458, -1229, 0, 1229, 2458, 3686,
};
constexpr int32_t kGainQ12[kNumSawsomeVoices] = {
    2048, 2867, 3482, 4096, 3482, 2867, 2048,
};

inline uint32_t U12ToQ12(uint16_t v) {
  if (v >= 4095U) return 4096U;
  return uint32_t(v);
}
}  // namespace

SawsomeEngine::SawsomeEngine() {
  Init();
}

void SawsomeEngine::Init() {
  for (uint8_t i = 0; i < kNumSawsomeVoices; ++i) {
    phases_[i] = uint32_t(i) * 0x1f123bb5U;
    tri_state_[i] = 0;
  }
}

void SawsomeEngine::OnSelected() {
  // Keep phase continuity.
}

void SawsomeEngine::ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) {
  SawsomeControlFrame& out = frame.sawsome;

  const uint32_t width_q12 = U12ToQ12(global.macro_x);
  const uint32_t detune_q12 = U12ToQ12(global.macro_y);
  out.alt = global.mode_alt;

  for (uint8_t i = 0; i < kNumSawsomeVoices; ++i) {
    int32_t ratio_q16 = 65536 + int32_t((int64_t(kDetuneOffsetQ16[i]) * int32_t(detune_q12)) >> 12);
    if (ratio_q16 < 32768) ratio_q16 = 32768;

    uint64_t inc = (uint64_t(global.pitch_inc) * uint32_t(ratio_q16)) >> 16;
    if (inc > 0x7FFFFFFFULL) inc = 0x7FFFFFFFULL;
    out.voice_phase_increment[i] = uint32_t(inc);

    int32_t pan_q12 = int32_t((int64_t(kPanQ12[i]) * int32_t(width_q12)) >> 12);
    int32_t gain_l_q12 = int32_t((int64_t(kGainQ12[i]) * (kUnityQ12 - pan_q12)) >> 13);
    int32_t gain_r_q12 = int32_t((int64_t(kGainQ12[i]) * (kUnityQ12 + pan_q12)) >> 13);

    if (gain_l_q12 < 0) gain_l_q12 = 0;
    if (gain_r_q12 < 0) gain_r_q12 = 0;
    out.voice_gain_l_q12[i] = int16_t(gain_l_q12);
    out.voice_gain_r_q12[i] = int16_t(gain_r_q12);
  }
}

int32_t SawsomeEngine::Clamp12(int32_t v) {
  if (v > 2047) return 2047;
  if (v < -2048) return -2048;
  return v;
}

int32_t SawsomeEngine::PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc) {
  int32_t saw = int32_t((phase >> 20) & 0x0FFFU) - 2048;
  if (phase_inc == 0) return saw;

  if (phase < phase_inc) {
    uint32_t t_q12 = uint32_t((uint64_t(phase) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = x + x - int32_t((int64_t(x) * x) >> 12) - 4096;
    saw -= corr >> 1;
  }

  uint32_t tail = 0xFFFFFFFFU - phase;
  if (tail < phase_inc) {
    uint32_t t_q12 = uint32_t((uint64_t(tail) << 12) / phase_inc);
    int32_t x = int32_t(t_q12);
    int32_t corr = int32_t((int64_t(x) * x) >> 12);
    saw += corr >> 1;
  }

  return Clamp12(saw);
}

int32_t SawsomeEngine::PolyBlepTriangleQ12(int voice_index, uint32_t phase, uint32_t phase_inc) {
  int32_t saw = PolyBlepSawQ12(phase, phase_inc);
  tri_state_[voice_index] += saw;
  tri_state_[voice_index] -= tri_state_[voice_index] >> 11;
  return Clamp12(tri_state_[voice_index] >> 5);
}

void SawsomeEngine::RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) {
  const SawsomeControlFrame& in = frame.sawsome;

  int32_t sum_l = 0;
  int32_t sum_r = 0;

  for (uint8_t i = 0; i < kNumSawsomeVoices; ++i) {
    phases_[i] += in.voice_phase_increment[i];

    int32_t s = in.alt ? PolyBlepTriangleQ12(i, phases_[i], in.voice_phase_increment[i])
                       : PolyBlepSawQ12(phases_[i], in.voice_phase_increment[i]);

    sum_l += int32_t((int64_t(s) * in.voice_gain_l_q12[i]) >> 12);
    sum_r += int32_t((int64_t(s) * in.voice_gain_r_q12[i]) >> 12);
  }

  out1 = Clamp12(sum_l >> 1);
  out2 = Clamp12(sum_r >> 1);
}

}  // namespace mtws
