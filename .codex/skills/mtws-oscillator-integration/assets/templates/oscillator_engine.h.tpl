#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class {{EngineClass}}Engine : public EngineInterface {
 public:
  // Creates the integrated engine with the shared sine lookup dependency.
  explicit {{EngineClass}}Engine(SineLUT* lut);

  // Resets runtime state to deterministic startup values.
  void Init() override;
  // Handles slot-selection refresh without resetting long-lived phase by default.
  void OnSelected() override;
  // Converts the shared global frame into the engine-specific control frame.
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  // Renders one stereo sample from the precomputed engine frame.
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  // Clamps one sample to the signed 12-bit board output domain.
  static int32_t Clamp12(int32_t v);

  // Shared sine lookup table used by the starter scaffold.
  SineLUT* lut_;
  // Phase accumulator in 0.32 format.
  uint32_t phase_;
};

}  // namespace mtws
