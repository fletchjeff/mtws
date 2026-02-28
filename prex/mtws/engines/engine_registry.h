#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/additive_engine.h"
#include "prex/mtws/engines/placeholder_engine.h"
#include "prex/mtws/engines/wavetable_engine.h"

namespace mtws {

class EngineRegistry {
 public:
  explicit EngineRegistry(SineLUT* lut);

  void Init();
  EngineInterface* Get(uint8_t slot);

 private:
  PlaceholderEngine va_placeholder_;
  WavetableEngine wavetable_;
  AdditiveEngine additive_;
  PlaceholderEngine waveshaping_placeholder_;
  PlaceholderEngine formant_placeholder_;
  PlaceholderEngine filtered_noise_placeholder_;
  EngineInterface* slots_[kNumOscillatorSlots];
};

}  // namespace mtws
