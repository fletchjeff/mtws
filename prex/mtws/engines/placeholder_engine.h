#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class PlaceholderEngine : public EngineInterface {
 public:
  enum Kind : uint8_t {
    VirtualAnalogue = 0,
    WaveShaping = 1,
    Formant = 2,
    FilteredNoise = 3,
  };

  PlaceholderEngine(SineLUT* lut, Kind kind);

  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  static inline int32_t Clamp12(int32_t v) {
    if (v < -2048) return -2048;
    if (v > 2047) return 2047;
    return v;
  }

  SineLUT* lut_;
  Kind kind_;
  uint32_t phase_;
  uint32_t phase_aux_;
  uint32_t noise_state_;
  int32_t noise_lp_;
};

}  // namespace mtws
