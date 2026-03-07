#pragma once

#include <cstdint>

namespace mtws {

constexpr uint8_t kNumOscillatorSlots = 6;
constexpr uint8_t kNumCumulusVoices = 16;
constexpr uint8_t kNumSawsomeVoices = 7;

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

struct SawsomeControlFrame {
  uint32_t voice_phase_increment[kNumSawsomeVoices];
  int16_t voice_gain_l_q12[kNumSawsomeVoices];
  int16_t voice_gain_r_q12[kNumSawsomeVoices];
  bool alt;
};

struct BenderControlFrame {
  uint32_t phase_increment;
  uint16_t fold_drive_q7;
  uint16_t fold_mix_q12;
  int16_t crush_amount;
  bool alt;
};

struct FloatableControlFrame {
  uint32_t phase_inc;
  uint8_t i00;
  uint8_t i10;
  uint8_t i01;
  uint8_t i11;
  uint8_t aa_level;
  uint16_t x_frac_q12;
  uint16_t y_frac_q12;
  bool alt;
};

struct CumulusControlFrame {
  uint32_t voice_phase_increment[kNumCumulusVoices];
  int32_t voice_gain_q12[kNumCumulusVoices];
  int32_t mix_norm_q12;
};

struct LosengeControlFrame {
  uint32_t phase_increment;
  uint32_t formant_increment[2][3];
  uint16_t formant_amplitude_q12[2][3];
  uint16_t voice_gain_q12;
};

struct DinSumControlFrame {
  uint16_t low_f_q15;
  uint16_t high_f_q15;
  uint16_t damping_q12;
  bool alt;
};

struct EngineControlFrame {
  SawsomeControlFrame sawsome;
  BenderControlFrame bender;
  FloatableControlFrame floatable;
  CumulusControlFrame cumulus;
  LosengeControlFrame losenge;
  DinSumControlFrame din_sum;
};

struct ControlFrame {
  GlobalControlFrame global;
  EngineControlFrame engine;
};

}  // namespace mtws
