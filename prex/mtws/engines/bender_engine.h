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
  static int32_t FoldFunction(int32_t x);
  static int32_t FoldIntegral(int32_t x);
  static int32_t AntiAliasedFold(int32_t x, int32_t& last_integral, int32_t& last_input);
  static int32_t CrushFunction(int32_t x, int32_t amount);
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);

  SineLUT* lut_;
  uint32_t phase_;
  int32_t last_fold_integral_pre_;
  int32_t last_fold_input_pre_;
  int32_t last_fold_integral_post_;
  int32_t last_fold_input_post_;
  bool last_alt_;
};

}  // namespace mtws
