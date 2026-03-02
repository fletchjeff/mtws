#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// CumulusSoloEngine is a 16-partial additive voice with centroid-driven spectral
// shaping. Control-rate code prepares per-partial increments/gains; audio-rate
// code only runs table lookups and fixed-point accumulation.
class CumulusSoloEngine {
 public:
  // Precomputed per-block data consumed by RenderSample.
  struct RenderFrame {
    // Per-partial oscillator increments in 0.32 phase units/sample.
    uint32_t voice_phase_increment[16];
    // Per-partial amplitude in Q12 gain (0..4096 nominal).
    int32_t voice_gain_q12[16];
    // Post-mix normalization gain in Q12.
    int32_t mix_norm_q12;
  };

  // Creates the engine with shared sine lookup table dependency.
  explicit CumulusSoloEngine(mtws::SineLUT* lut);

  // Resets phases and smoothing states.
  void Init();
  // Builds one block RenderFrame from incoming control/macro values.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Unity gain in Q12.
  static constexpr uint32_t kUnityQ12 = 4096U;
  // Minimum allowed partial ratio in Q16 (0.1x).
  static constexpr int32_t kMinRatioQ16 = 6554;
  // Maximum allowed partial ratio in Q16 (16x).
  static constexpr int32_t kMaxRatioQ16 = 1048576;
  // Absolute Nyquist-safe phase increment clamp.
  static constexpr uint32_t kNyquistPhaseInc = 0x7FFFFFFFU;
  // Additional anti-alias guard where partials are muted.
  static constexpr uint32_t kPartialMaxPhaseInc = 0x7F000000U;
  // One-pole smoothing strength for ratio changes.
  static constexpr int kRatioSmoothShift = 3;
  // One-pole smoothing strength for gain changes.
  static constexpr int kGainSmoothShift = 3;
  // Headroom shift after summing 8 voices per stereo bus.
  static constexpr int kRenderSumShift = 4;
  // Overall master ceiling for auto-normalized mix gain.
  static constexpr uint32_t kCumulusMasterGainQ12 = 32768U;
  // Keeps partial gain roughly balanced before bus normalization.
  static constexpr uint32_t kUniformPartialGainQ12 = 8192U;

  // Clamps to signed 12-bit output range.
  static int32_t Clamp12(int32_t v);
  // Q12 interpolation utility where t_q12 spans 0..4096.
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  // Integer smoothing step toward target (`shift` controls slew).
  static int32_t SmoothToward(int32_t current, int32_t target, int shift);
  // Reflect-folds ratio values into [lo, hi] range in Q16.
  static int32_t FoldReflectQ16(int32_t x, int32_t lo, int32_t hi);
  // Broad centroid weighting (1 / (1 + distance)).
  static int32_t CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12);
  // Narrow triangular centroid weighting (zero outside +/-1 partial).
  static int32_t CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12);
  // Macro-driven blend of narrow/base/unity centroid shapes.
  static int32_t ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y);
  // Harmonic ratio helper: partial i -> (i+1) in Q16.
  static int32_t HarmonicRatioQ16(int i) { return int32_t((i + 1) << 16); }

  // Shared sine lookup table (owned externally).
  mtws::SineLUT* lut_;
  // Per-partial phase accumulators in 0.32.
  uint32_t phases_[16];
  // Smoothed per-partial frequency ratio state (Q16).
  mutable int32_t smoothed_ratio_q16_[16];
  // Smoothed per-partial gain state (Q12).
  mutable int32_t smoothed_gain_q12_[16];
};
