#pragma once

#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class WavetableEngine : public EngineInterface {
 public:
  WavetableEngine();

  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  static constexpr uint32_t kNumWaveforms = 64;
  static constexpr uint32_t kSamplesPerWave = 512;

  uint32_t phase_;
  int32_t out1_delay_[2];
};

}  // namespace mtws
