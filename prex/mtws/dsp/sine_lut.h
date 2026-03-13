#pragma once

#include <cstdint>
#include <cmath>
#include "pico.h"

namespace mtws {

class SineLUT {
 public:
  static constexpr uint32_t kTableSize = 512;
  static constexpr uint32_t kTableMask = kTableSize - 1;

  SineLUT() {
    for (uint32_t i = 0; i < kTableSize; ++i) {
      table_[i] = int16_t(32000.0 * std::sin(2.0 * M_PI * double(i) / double(kTableSize)));
    }
  }

  inline int32_t __not_in_flash_func(LookupLinear)(uint32_t phase) const {
    uint32_t index = phase >> 23;
    int32_t frac = int32_t((phase & 0x7FFFFFU) >> 7);  // Q16
    int32_t s1 = table_[index];
    int32_t s2 = table_[(index + 1U) & kTableMask];
    return (s2 * frac + s1 * (65536 - frac)) >> 20;
  }

 private:
  int16_t table_[kTableSize];
};

}  // namespace mtws
