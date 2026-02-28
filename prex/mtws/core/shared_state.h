#pragma once

#include <cstdint>

namespace mtws {

constexpr uint8_t kNumOscillatorSlots = 6;
constexpr uint8_t kNumAdditiveVoices = 16;

struct UISnapshot {
  uint16_t knob_main;
  uint16_t knob_x;
  uint16_t knob_y;
  int16_t cv1;
  int16_t cv2;
  int16_t audio1;
  int16_t audio2;
  bool audio1_connected;
  bool audio2_connected;
  bool cv2_connected;
  bool pulse1_connected;
  bool pulse1_high;
  uint8_t selected_slot;
  bool panel_alt_latched;
};

struct MidiState {
  bool note_active;
  uint8_t current_note;
  uint8_t last_note;
  uint8_t cc1;
  uint8_t cc74;
  uint32_t note_on_counter;
};

struct GlobalControlFrame {
  uint8_t selected_slot;
  bool mode_alt;
  uint16_t macro_x;
  uint16_t macro_y;
  uint32_t pitch_inc;
  uint16_t vca_gain_q12;
  uint8_t current_midi_note;
  uint8_t last_midi_note;
  bool midi_note_active;
  uint32_t note_on_counter;
};

struct AdditiveControlFrame {
  uint32_t voice_phase_increment[kNumAdditiveVoices];
  int32_t voice_gain_q12[kNumAdditiveVoices];
  int32_t mix_norm_q12;
};

struct WavetableControlFrame {
  uint32_t phase_inc;
  uint32_t wave_pos_q16;
  uint16_t macro_y;
  bool alt;
};

struct PlaceholderControlFrame {
  uint32_t phase_inc;
  uint16_t macro_x;
  uint16_t macro_y;
  uint8_t kind;
  bool alt;
};

struct EngineControlFrame {
  AdditiveControlFrame additive;
  WavetableControlFrame wavetable;
  PlaceholderControlFrame placeholder;
};

struct ControlFrame {
  GlobalControlFrame global;
  EngineControlFrame engine;
};

}  // namespace mtws
