#include "prex/solo_common/solo_control_router.h"

namespace solo_common {

namespace {
constexpr uint32_t kBasePhaseInc10Hz = 894785U;      // 10 * 2^32 / 48k
constexpr uint32_t kMaxPhaseInc10kHz = 894784853U;   // 10000 * 2^32 / 48k

constexpr uint32_t kExp2Q16[129] = {
     65536,  65892,  66250,  66609,  66971,  67335,  67700,  68068,  68438,  68809,  69183,  69558,  69936,  70316,  70698,  71082,
     71468,  71856,  72246,  72638,  73032,  73429,  73828,  74229,  74632,  75037,  75444,  75854,  76266,  76680,  77096,  77515,
     77936,  78359,  78785,  79212,  79642,  80075,  80510,  80947,  81386,  81828,  82273,  82719,  83169,  83620,  84074,  84531,
     84990,  85451,  85915,  86382,  86851,  87322,  87796,  88273,  88752,  89234,  89719,  90206,  90696,  91188,  91684,  92181,
     92682,  93185,  93691,  94200,  94711,  95226,  95743,  96263,  96785,  97311,  97839,  98370,  98905,  99442,  99982, 100524,
    101070, 101619, 102171, 102726, 103283, 103844, 104408, 104975, 105545, 106118, 106694, 107274, 107856, 108442, 109031, 109623,
    110218, 110816, 111418, 112023, 112631, 113243, 113858, 114476, 115098, 115723, 116351, 116983, 117618, 118257, 118899, 119544,
    120194, 120846, 121502, 122162, 122825, 123492, 124163, 124837, 125515, 126197, 126882, 127571, 128263, 128960, 129660, 130364,
    131072,
};

}  // namespace

uint16_t SoloControlRouter::ClampU12(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return uint16_t(v);
}

uint32_t SoloControlRouter::BasePhaseIncrementFromPitchCode(uint32_t pitch_code) {
  if (pitch_code > 4095U) pitch_code = 4095U;

  uint32_t pos = (pitch_code * 5122U) >> 12;
  if (pos > 5120U) pos = 5120U;

  uint32_t octave_int = pos >> 9;
  uint32_t sub = pos & 0x1FFU;

  uint32_t inc = kBasePhaseInc10Hz << octave_int;

  uint32_t idx = sub >> 2;
  uint32_t frac = sub & 0x3U;
  uint32_t m1 = kExp2Q16[idx];
  uint32_t m2 = kExp2Q16[idx + 1U];
  uint32_t mult = m1 + (((m2 - m1) * frac) >> 2);

  uint32_t out = uint32_t((uint64_t(inc) * mult) >> 16);
  if (out < kBasePhaseInc10Hz) out = kBasePhaseInc10Hz;
  if (out > kMaxPhaseInc10kHz) out = kMaxPhaseInc10kHz;
  return out;
}

ControlFrame SoloControlRouter::Build(const UISnapshot& ui) {
  ControlFrame out{};

  out.pitch_inc = BasePhaseIncrementFromPitchCode(ui.knob_main);

  uint16_t x_local = ui.knob_x;
  if (ui.audio1_connected) {
    uint16_t audio_uni = ClampU12(int32_t(ui.audio1) + 2048);
    x_local = uint16_t((uint32_t(audio_uni) * ui.knob_x + 2048U) >> 12);
  }

  uint16_t y_local = ui.knob_y;
  if (ui.audio2_connected) {
    uint16_t audio_uni = ClampU12(int32_t(ui.audio2) + 2048);
    y_local = uint16_t((uint32_t(audio_uni) * ui.knob_y + 2048U) >> 12);
  }

  out.macro_x = x_local;
  out.macro_y = y_local;
  out.alt = ui.alt;
  return out;
}

}  // namespace solo_common
