#pragma once

#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/bender_engine.h"
#include "prex/mtws/engines/cumulus_engine.h"
#include "prex/mtws/engines/din_sum_engine.h"
#include "prex/mtws/engines/floatable_engine.h"
#include "prex/mtws/engines/losenge_engine.h"
#include "prex/mtws/engines/sawsome_engine.h"

namespace mtws {

class EngineRegistry {
 public:
  explicit EngineRegistry(SineLUT* lut);

  void Init();
  EngineInterface* Get(uint8_t slot);

 private:
  SawsomeEngine sawsome_;
  BenderEngine bender_;
  FloatableEngine floatable_;
  CumulusEngine cumulus_;
  LosengeEngine losenge_;
  DinSumEngine din_sum_;

  EngineInterface* slots_[kNumOscillatorSlots];
};

}  // namespace mtws
