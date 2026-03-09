#include "prex/solo_common/solo_control_router.h"

namespace solo_common {

namespace {
constexpr uint32_t kBasePhaseInc10Hz = 894785U;      // 10 * 2^32 / 48k
constexpr uint32_t kMaxPhaseInc10kHz = 894784853U;   // 10000 * 2^32 / 48k
constexpr uint32_t kMaxU12 = 4095U;
// ComputerCard audio/CV signals span approximately +/-6 V over -2048..2047.
// Solo preview should use the same practical modulation range as `mtws`, where
// +/-5 V covers the full macro span and hotter sources saturate.
constexpr int32_t kFiveVoltInputCode = 1706;

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
  if (v > int32_t(kMaxU12)) return kMaxU12;
  return uint16_t(v);
}

// Clamps one bipolar board-domain input code into the intended -5 V..+5 V
// modulation range so hotter sources behave like saturated CV, not overflow.
static inline int32_t ClampBipolarFiveVoltInput(int32_t input_code) {
  if (input_code < -kFiveVoltInputCode) return -kFiveVoltInputCode;
  if (input_code > kFiveVoltInputCode) return kFiveVoltInputCode;
  return input_code;
}

// Maps a bipolar -5 V..+5 V input code into the unsigned macro domain.
// Endpoints are -5 V -> 0, 0 V -> ~2048, +5 V -> 4095.
static inline uint16_t MapBipolarFiveVoltInputToU12(int32_t input_code) {
  const uint32_t shifted_code = uint32_t(ClampBipolarFiveVoltInput(input_code) + kFiveVoltInputCode);
  return uint16_t((shifted_code * kMaxU12 + uint32_t(kFiveVoltInputCode)) / (2U * uint32_t(kFiveVoltInputCode)));
}

// Applies a 0..4095 attenuation amount to a 0..4095 source and rounds to the
// nearest output code so full-scale attenuation preserves full-scale inputs.
static inline uint16_t ApplyU12Attenuation(uint16_t source_u12, uint16_t amount_u12) {
  return uint16_t((uint32_t(source_u12) * uint32_t(amount_u12) + (kMaxU12 / 2U)) / kMaxU12);
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
    x_local = ApplyU12Attenuation(MapBipolarFiveVoltInputToU12(ui.audio1), ui.knob_x);
  }

  uint16_t y_local = ui.knob_y;
  if (ui.audio2_connected) {
    y_local = ApplyU12Attenuation(MapBipolarFiveVoltInputToU12(ui.audio2), ui.knob_y);
  }

  out.macro_x = x_local;
  out.macro_y = y_local;
  out.alt = ui.alt;
  return out;
}

}  // namespace solo_common
