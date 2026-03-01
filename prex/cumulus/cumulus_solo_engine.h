#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

class CumulusSoloEngine {
 public:
  struct RenderFrame {
    uint32_t voice_phase_increment[16];
    int32_t voice_gain_q12[16];
    int32_t mix_norm_q12;
  };

  explicit CumulusSoloEngine(mtws::SineLUT* lut);

  void Init();
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  static constexpr uint32_t kUnityQ12 = 4096U;
  static constexpr int32_t kMinRatioQ16 = 6554;
  static constexpr int32_t kMaxRatioQ16 = 1048576;
  static constexpr uint32_t kNyquistPhaseInc = 0x7FFFFFFFU;
  static constexpr uint32_t kPartialMaxPhaseInc = 0x7F000000U;
  static constexpr int kRatioSmoothShift = 3;
  static constexpr int kGainSmoothShift = 3;
  static constexpr int kRenderSumShift = 4;
  static constexpr uint32_t kCumulusMasterGainQ12 = 32768U;
  static constexpr uint32_t kUniformPartialGainQ12 = 8192U;

  static int32_t Clamp12(int32_t v);
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  static int32_t SmoothToward(int32_t current, int32_t target, int shift);
  static int32_t FoldReflectQ16(int32_t x, int32_t lo, int32_t hi);
  static int32_t CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12);
  static int32_t CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12);
  static int32_t ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y);
  static int32_t HarmonicRatioQ16(int i) { return int32_t((i + 1) << 16); }

  mtws::SineLUT* lut_;
  uint32_t phases_[16];
  mutable int32_t smoothed_ratio_q16_[16];
  mutable int32_t smoothed_gain_q12_[16];
};
