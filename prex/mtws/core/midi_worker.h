#pragma once

#include <cstdint>

#include "prex/mtws/core/shared_state.h"

namespace mtws {

class MIDIWorker {
 public:
  MIDIWorker();

  void Init();
  void Poll();
  MidiState Snapshot() const;

 private:
  void HandleMIDIMessage(const uint8_t* packet);

  MidiState state_;
};

}  // namespace mtws
