#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

// FloatableSoloEngine prepares compact morph state at control rate, then reads
// directly from the curated source-wave bank at audio rate. This now matches
// the integrated `mtws` engine instead of rendering finished output tables.
class FloatableSoloEngine {
 public:
  // Per-block values used during audio-rate rendering.
  struct RenderFrame {
    // Base oscillator phase increment in 0.32 phase units/sample.
    uint32_t phase_inc;
    // Selects the active curated source bank. `0` = normal bank 1, `1` = alt
    // bank 4. Both outputs read from the same active bank.
    uint8_t use_alt_banks;
    // Base source-wave index for Out1 morph in the range 0..14.
    uint8_t out1_wave_index;
    // Q12 morph fraction for Out1 in the range 0..4096.
    uint16_t out1_wave_frac_q12;
    // Base source-wave index for Out2 morph in the range 0..14.
    uint8_t out2_wave_index;
    // Q12 morph fraction for Out2 in the range 0..4096.
    uint16_t out2_wave_frac_q12;
  };

  // Creates the engine with shared LUT dependency (kept for interface parity).
  explicit FloatableSoloEngine(mtws::SineLUT* lut);

  // Resets oscillator phase.
  void Init();
  // Builds the next pair of final output tables from the latest control frame.
  void BuildRenderFrame(const solo_common::ControlFrame& control, RenderFrame& out) const;
  // Renders one stereo sample in signed 12-bit output domain.
  void RenderSample(const RenderFrame& frame, int32_t& out1, int32_t& out2);

 private:
  // Q12 interpolation helper where t_q12 is 0..4096.
  static int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12);
  // Converts one 0..4095 control axis into a source-wave index plus Q12
  // interpolation fraction between adjacent waves.
  static void ComputeWaveSelection(uint32_t axis_code, uint8_t& wave_index, uint16_t& wave_frac_q12);
  // Renders one audio sample from the selected source bank by interpolating in
  // table phase first, then morphing between adjacent source waves.
  static int32_t RenderMorphedBankSample(const int16_t (*source_waves)[256],
                                         uint32_t wave_index,
                                         uint32_t wave_frac_q12,
                                         uint32_t sample_index,
                                         uint32_t sample_next,
                                         uint32_t sample_frac_q12);

  // Kept for API consistency across engines.
  mtws::SineLUT* lut_;
  // Oscillator phase accumulator in 0.32 format.
  uint32_t phase_;
};
