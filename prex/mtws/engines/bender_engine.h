#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class BenderEngine : public EngineInterface {
 public:
  explicit BenderEngine(SineLUT* lut);

  void Init() override;
  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  static int32_t Clamp12(int32_t v);
  static int32_t TriangleQ12(uint32_t phase);
  static int32_t SawQ12(uint32_t phase);
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  static int32_t Fold12(int32_t x);

  SineLUT* lut_;
  uint32_t phase_;
};

}  // namespace mtws
