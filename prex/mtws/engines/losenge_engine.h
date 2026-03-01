#pragma once

#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class LosengeEngine : public EngineInterface {
 public:
  LosengeEngine();

  void Init() override;
  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  struct SVFState {
    int32_t lp;
    int32_t bp;
  };

  static int32_t Clamp12(int32_t v);
  static uint16_t HzToFQ15(uint32_t hz);
  static uint32_t LerpU32(uint32_t a, uint32_t b, uint16_t t_u12);
  static int32_t SawQ12(uint32_t phase);
  static int32_t SquareQ12(uint32_t phase);
  static int32_t SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);

  uint32_t phase_;
  SVFState out1_a_;
  SVFState out1_b_;
  SVFState out2_a_;
  SVFState out2_b_;
};

}  // namespace mtws
