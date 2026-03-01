#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class BenderSoloEngine {
 public:
  struct RenderFrame {
    uint32_t phase_increment;
    uint16_t macro_x;
    uint16_t macro_y;
    bool alt;
  };

  explicit BenderSoloEngine(mtws::SineLUT* lut);

  void Init();
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  static int32_t Clamp12(int32_t v);
  static int32_t TriangleQ12(uint32_t phase);
  static int32_t SawQ12(uint32_t phase);
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  static int32_t Fold12(int32_t x);

  mtws::SineLUT* lut_;
  uint32_t phase_;
};
