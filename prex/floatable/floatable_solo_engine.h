#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class FloatableSoloEngine {
 public:
  struct RenderFrame {
    uint32_t phase_inc;
    uint8_t i00;
    uint8_t i10;
    uint16_t x_frac_q12;
    bool alt;
  };

  explicit FloatableSoloEngine(mtws::SineLUT* lut);

  void Init();
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);

  mtws::SineLUT* lut_;
  uint32_t phase_;
};
