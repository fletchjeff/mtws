#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class DinSumSoloEngine {
 public:
  struct RenderFrame {
    uint16_t low_f_q15;
    uint16_t high_f_q15;
    uint16_t damping_q12;
    bool alt;
  };

  explicit DinSumSoloEngine(mtws::SineLUT* lut);

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
  static int32_t SVFHighpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);
  static int32_t SVFLowpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);
  static int32_t SVFBandpass(int32_t in, uint16_t f_q15, uint16_t damping_q12, SVFState& state);

  mtws::SineLUT* lut_;
  uint32_t noise_state_;
  int32_t pink_state_;
  SVFState bp1_state_;
  SVFState bp2_state_;
  SVFState hp_state_;
  SVFState lp_state_;
};
