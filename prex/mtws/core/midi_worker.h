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
  void HandleChannelMessage(uint8_t status, uint8_t data1, uint8_t data2);
  void ConsumeMIDIByte(uint8_t byte);

  MidiState state_;
  uint8_t running_status_;
  uint8_t data_bytes_[2];
  uint8_t data_count_;
  uint8_t needed_data_bytes_;
  bool in_sysex_;
};

}  // namespace mtws
