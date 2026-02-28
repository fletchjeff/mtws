#include "prex/mtws/core/midi_worker.h"

#include "tusb.h"

namespace mtws {

MIDIWorker::MIDIWorker() {
  state_.note_active = false;
  state_.current_note = 60;
  state_.last_note = 60;
  state_.cc1 = 0;
  state_.cc74 = 0;
  state_.note_on_counter = 0;
  running_status_ = 0;
  data_bytes_[0] = 0;
  data_bytes_[1] = 0;
  data_count_ = 0;
  needed_data_bytes_ = 0;
  in_sysex_ = false;
}

void MIDIWorker::Init() {
  tusb_init();
}

void MIDIWorker::HandleChannelMessage(uint8_t status, uint8_t data1, uint8_t data2) {
  uint8_t status_nibble = status & 0xF0U;
  uint8_t channel = status & 0x0FU;

  // Channel-1 strict. MIDI channel 1 is zero-based channel 0.
  if (channel != 0U) return;

  switch (status_nibble) {
    case 0x90: {  // note on
      uint8_t note = data1 & 0x7FU;
      uint8_t velocity = data2 & 0x7FU;
      if (velocity == 0U) {
        if (state_.note_active && note == state_.current_note) {
          state_.note_active = false;
        }
      } else {
        state_.note_active = true;
        state_.current_note = note;
        state_.last_note = note;
        state_.note_on_counter++;
      }
      break;
    }

    case 0x80: {  // note off
      uint8_t note = data1 & 0x7FU;
      if (state_.note_active && note == state_.current_note) {
        state_.note_active = false;
      }
      break;
    }

    case 0xB0: {  // control change
      uint8_t cc = data1 & 0x7FU;
      uint8_t value = data2 & 0x7FU;
      if (cc == 1U) {
        state_.cc1 = value;
      } else if (cc == 74U) {
        state_.cc74 = value;
      }
      break;
    }

    default:
      break;
  }
}

void MIDIWorker::ConsumeMIDIByte(uint8_t byte) {
  if (byte & 0x80U) {
    // Realtime message bytes can appear anywhere; ignore without resetting parser state.
    if (byte >= 0xF8U) return;

    if (byte == 0xF0U) {
      in_sysex_ = true;
      running_status_ = 0;
      data_count_ = 0;
      needed_data_bytes_ = 0;
      return;
    }

    if (byte == 0xF7U) {
      in_sysex_ = false;
      data_count_ = 0;
      needed_data_bytes_ = 0;
      return;
    }

    if (in_sysex_) return;

    // Ignore non-channel status bytes and clear running status.
    if (byte >= 0xF0U) {
      running_status_ = 0;
      data_count_ = 0;
      needed_data_bytes_ = 0;
      return;
    }

    running_status_ = byte;
    data_count_ = 0;
    uint8_t status_nibble = byte & 0xF0U;
    needed_data_bytes_ = (status_nibble == 0xC0U || status_nibble == 0xD0U) ? 1U : 2U;
    return;
  }

  if (in_sysex_ || running_status_ == 0U || needed_data_bytes_ == 0U) return;

  if (data_count_ < 2U) {
    data_bytes_[data_count_++] = byte & 0x7FU;
  }

  if (data_count_ >= needed_data_bytes_) {
    if (needed_data_bytes_ == 2U) {
      HandleChannelMessage(running_status_, data_bytes_[0], data_bytes_[1]);
    }
    // Keep running status; parse next message data bytes.
    data_count_ = 0;
  }
}

void MIDIWorker::Poll() {
  tud_task();

  uint8_t buffer[64];
  while (tud_midi_available()) {
    uint32_t bytes_read = tud_midi_stream_read(buffer, sizeof(buffer));
    for (uint32_t i = 0; i < bytes_read; ++i) {
      ConsumeMIDIByte(buffer[i]);
    }
  }
}

MidiState MIDIWorker::Snapshot() const {
  return state_;
}

}  // namespace mtws
