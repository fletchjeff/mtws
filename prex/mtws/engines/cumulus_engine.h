#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class CumulusEngine : public EngineInterface {
 public:
  explicit CumulusEngine(SineLUT* lut);

  void Init() override;
  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  static constexpr uint32_t kUnityQ12 = 4096U;
  static constexpr int32_t kMinRatioQ16 = 6554;      // 0.1x
  static constexpr int32_t kMaxRatioQ16 = 1048576;   // 16x
  static constexpr uint32_t kNyquistPhaseInc = 0x7FFFFFFFU;
  static constexpr uint32_t kPartialMaxPhaseInc = 0x7F000000U;  // ~23.8kHz @ 48kHz (just below Nyquist)
  static constexpr int kRatioSmoothShift = 3;
  static constexpr int kGainSmoothShift = 3;
  static constexpr int kRenderSumShift = 4;  // Fixed headroom for odd/even bus sums.
  static constexpr uint32_t kCumulusMasterGainQ12 = 32768U;  // Master makeup target, auto-capped per frame.
  static constexpr uint32_t kUniformPartialGainQ12 = 8192U;   // Uniform per-partial gain (2x).

  inline int32_t HarmonicRatioQ16(int i) const { return int32_t((i + 1) << 16); }

  static int32_t CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12);
  static int32_t CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12);
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  static int32_t SmoothToward(int32_t current, int32_t target, int shift);
  static int32_t FoldReflectQ16(int32_t x, int32_t lo, int32_t hi);
  static int32_t ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y);

  SineLUT* lut_;
  uint32_t phases_[kNumCumulusVoices];
  int32_t smoothed_ratio_q16_[kNumCumulusVoices];
  int32_t smoothed_gain_q12_[kNumCumulusVoices];
};

}  // namespace mtws
