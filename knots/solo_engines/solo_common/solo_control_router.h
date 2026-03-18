#pragma once

#include <cstdint>

namespace solo_common {

struct UISnapshot {
  uint16_t knob_main;
  uint16_t knob_x;
  uint16_t knob_y;
  int16_t audio1;
  int16_t audio2;
  bool audio1_connected;
  bool audio2_connected;
  bool alt;
};

struct ControlFrame {
  uint32_t pitch_inc;
  uint16_t macro_x;
  uint16_t macro_y;
  bool alt;
};

class SoloControlRouter {
 public:
  static ControlFrame Build(const UISnapshot& ui);

 private:
  static uint32_t BasePhaseIncrementFromPitchCode(uint32_t pitch_code);
  static uint16_t ClampU12(int32_t v);
};

}  // namespace solo_common
