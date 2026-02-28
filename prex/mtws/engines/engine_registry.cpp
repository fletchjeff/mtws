#include "prex/mtws/engines/engine_registry.h"

namespace mtws {

EngineRegistry::EngineRegistry(SineLUT* lut)
    : va_placeholder_(lut, PlaceholderEngine::VirtualAnalogue),
      wavetable_(),
      additive_(lut),
      waveshaping_placeholder_(lut, PlaceholderEngine::WaveShaping),
      formant_placeholder_(lut, PlaceholderEngine::Formant),
      filtered_noise_placeholder_(lut, PlaceholderEngine::FilteredNoise),
      slots_{
          &va_placeholder_,
          &wavetable_,
          &additive_,
          &waveshaping_placeholder_,
          &formant_placeholder_,
          &filtered_noise_placeholder_,
      } {}

void EngineRegistry::Init() {
  for (uint8_t i = 0; i < kNumOscillatorSlots; ++i) {
    slots_[i]->Init();
  }
}

EngineInterface* EngineRegistry::Get(uint8_t slot) {
  if (slot >= kNumOscillatorSlots) {
    slot = 0;
  }
  return slots_[slot];
}

}  // namespace mtws
