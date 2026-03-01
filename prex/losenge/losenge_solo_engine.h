#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class LosengeSoloEngine {
 public:
  struct RenderFrame {
    uint32_t phase_increment;
    uint16_t out1_f1_q15;
    uint16_t out1_f2_q15;
    uint16_t out2_f1_q15;
    uint16_t out2_f2_q15;
    uint16_t damping_q12;
    bool alt;
  };

  explicit LosengeSoloEngine(mtws::SineLUT* lut);

  void Init();
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

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

  mtws::SineLUT* lut_;
  uint32_t phase_;
  SVFState out1_a_;
  SVFState out1_b_;
  SVFState out2_a_;
  SVFState out2_b_;
};
