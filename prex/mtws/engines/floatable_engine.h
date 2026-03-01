#pragma once

#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class FloatableEngine : public EngineInterface {
 public:
  FloatableEngine();

  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  static constexpr uint32_t kNumWaveforms = 64;
  static constexpr uint32_t kSamplesPerWave = 512;
  static constexpr uint32_t kOut2FadeStartPhase = 0xC0000000U;  // last quarter-cycle

  uint32_t phase_;
  bool out2_pair_valid_;
  uint8_t out2_active_l_;
  uint8_t out2_active_r_;
  bool out2_pending_valid_;
  uint8_t out2_pending_l_;
  uint8_t out2_pending_r_;
  bool out2_was_in_fade_window_;
};

}  // namespace mtws
