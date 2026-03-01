#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class SawsomeSoloEngine {
 public:
  struct RenderFrame {
    uint32_t phase_increment[7];
    int16_t gain_l_q12[7];
    int16_t gain_r_q12[7];
    bool alt;
  };

  explicit SawsomeSoloEngine(mtws::SineLUT* lut);

  void Init();
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  static int32_t Clamp12(int32_t v);
  static int32_t PolyBlepSawQ12(uint32_t phase, uint32_t phase_inc);
  int32_t PolyBlepTriangleQ12(int voice_index, uint32_t phase, uint32_t phase_inc);

  mtws::SineLUT* lut_;
  uint32_t phases_[7];
  int32_t tri_state_[7];
};
