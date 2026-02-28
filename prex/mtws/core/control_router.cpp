#include "prex/mtws/core/control_router.h"

namespace mtws {

namespace {
constexpr uint32_t kBasePhaseInc10Hz = 894785U;  // 10 * 2^32 / 48k
constexpr uint32_t kMaxPhaseInc10kHz = 894784853U;  // 10000 * 2^32 / 48k
constexpr uint32_t kUnityQ12 = 4096U;

// 2^(x) for x in [0,1] on 1/128 grid, Q16 format.
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

// 2^(n/12) for n=0..11 in Q16.
constexpr uint32_t kSemitoneQ16[12] = {
    65536, 69433, 73561, 77936, 82575, 87499, 92729, 98289, 104202, 110496, 117202, 124354,
};

inline uint16_t ClampU12(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return uint16_t(v);
}

}  // namespace

uint32_t ControlRouter::BasePhaseIncrementFromPitchCode(uint32_t pitch_code) {
  if (pitch_code > 4095U) pitch_code = 4095U;

  uint32_t pos = (pitch_code * 5122U) >> 12;  // 10 octaves, 512 steps/oct
  if (pos > 5120U) pos = 5120U;

  uint32_t octave_int = pos >> 9;
  uint32_t sub = pos & 0x1FFU;

  uint32_t inc = kBasePhaseInc10Hz << octave_int;

  uint32_t idx = sub >> 2;
  uint32_t frac = sub & 0x3U;
  uint32_t m1 = kExp2Q16[idx];
  uint32_t m2 = kExp2Q16[idx + 1U];
  uint32_t mult = m1 + (((m2 - m1) * frac) >> 2);

  return uint32_t((uint64_t(inc) * mult) >> 16);
}

uint32_t ControlRouter::ApplySemitoneToPhaseInc(uint32_t base_inc, int semitone) {
  if (base_inc == 0 || semitone == 0) return base_inc;

  if (semitone > 96) semitone = 96;
  if (semitone < -96) semitone = -96;

  int octave = semitone / 12;
  int rem = semitone % 12;
  if (rem < 0) {
    rem += 12;
    octave -= 1;
  }

  uint64_t scaled = (uint64_t(base_inc) * kSemitoneQ16[rem]) >> 16;

  if (octave > 0) {
    if (octave >= 16) {
      return 0x7FFFFFFFU;
    }
    scaled <<= octave;
  } else if (octave < 0) {
    int down = -octave;
    if (down >= 63) {
      scaled = 0;
    } else {
      scaled >>= down;
    }
  }

  if (scaled > 0x7FFFFFFFULL) return 0x7FFFFFFFU;
  return uint32_t(scaled);
}

GlobalControlFrame ControlRouter::BuildGlobalFrame(const UISnapshot& ui, const MidiState& midi) {
  GlobalControlFrame out{};

  out.selected_slot = (ui.selected_slot < kNumOscillatorSlots) ? ui.selected_slot : 0;

  int32_t pitch_local = int32_t(ui.knob_main) + int32_t(ui.cv1);
  uint16_t pitch_code = ClampU12(pitch_local);

  uint32_t base_inc = BasePhaseIncrementFromPitchCode(pitch_code);
  int semitone = midi.note_active ? (int(midi.current_note) - 60) : 0;
  out.pitch_inc = ApplySemitoneToPhaseInc(base_inc, semitone);
  if (out.pitch_inc < kBasePhaseInc10Hz) out.pitch_inc = kBasePhaseInc10Hz;
  if (out.pitch_inc > kMaxPhaseInc10kHz) out.pitch_inc = kMaxPhaseInc10kHz;

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

  uint32_t macro_x = uint32_t(x_local) + (uint32_t(midi.cc1) << 5);
  uint32_t macro_y = uint32_t(y_local) + (uint32_t(midi.cc74) << 5);
  if (macro_x > 4095U) macro_x = 4095U;
  if (macro_y > 4095U) macro_y = 4095U;
  out.macro_x = uint16_t(macro_x);
  out.macro_y = uint16_t(macro_y);

  out.mode_alt = ui.pulse1_connected ? ui.pulse1_high : ui.panel_alt_latched;

  if (ui.cv2_connected) {
    uint16_t cv2_uni = ClampU12(int32_t(ui.cv2) + 2048);
    out.vca_gain_q12 = uint16_t((uint32_t(cv2_uni) * kUnityQ12 + 2047U) / 4095U);
  } else {
    out.vca_gain_q12 = kUnityQ12;
  }

  out.midi_note_active = midi.note_active;
  out.current_midi_note = midi.current_note;
  out.last_midi_note = midi.last_note;
  out.note_on_counter = midi.note_on_counter;

  return out;
}

}  // namespace mtws
