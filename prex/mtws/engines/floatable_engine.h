#pragma once

#include "prex/mtws/engines/engine_interface.h"

namespace mtws {

class FloatableEngine : public EngineInterface {
 public:
  FloatableEngine();

  void OnSelected() override;
  void ControlTick(const GlobalControlFrame& global, EngineControlFrame& frame) override;
  void RenderSample(const EngineControlFrame& frame, int32_t& out1, int32_t& out2) override;

 private:
  // Q12 interpolation helper where `t_q12 = 0` returns `a` and `4096` returns `b`.
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);

  static constexpr uint32_t kNumSourceWaves = 16U;
  static constexpr uint32_t kInterpolationCells = kNumSourceWaves - 1U;
  static constexpr uint32_t kRenderedTableSize = 256U;
  static constexpr uint32_t kRenderedTableMask = kRenderedTableSize - 1U;
  static constexpr uint32_t kRenderedTableIndexShift = 24U;
  static constexpr uint32_t kRenderedTableFracShift = 12U;

  uint32_t phase_;
  // Cached 256-point render targets copied into each published control frame.
  // These let the control core update X and Y on alternating ticks without
  // leaving one output table uninitialized in the double-buffer handoff.
  int16_t cached_rendered_out1_[kRenderedTableSize];
  int16_t cached_rendered_out2_[kRenderedTableSize];
  bool caches_primed_;
  bool render_out1_on_next_tick_;
};

}  // namespace mtws
