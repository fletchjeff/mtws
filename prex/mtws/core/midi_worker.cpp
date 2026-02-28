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
}

void MIDIWorker::Init() {
  tusb_init();
}

void MIDIWorker::HandleMIDIMessage(const uint8_t* packet) {
  uint8_t status = packet[0] & 0xF0U;
  uint8_t channel = packet[0] & 0x0FU;

  // Channel-1 strict. MIDI channel 1 is zero-based channel 0.
  if (channel != 0U) return;

  switch (status) {
    case 0x90: {  // note on
      uint8_t note = packet[1] & 0x7FU;
      uint8_t velocity = packet[2] & 0x7FU;
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
      uint8_t note = packet[1] & 0x7FU;
      if (state_.note_active && note == state_.current_note) {
        state_.note_active = false;
      }
      break;
    }

    case 0xB0: {  // control change
      uint8_t cc = packet[1] & 0x7FU;
      uint8_t value = packet[2] & 0x7FU;
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

void MIDIWorker::Poll() {
  tud_task();

  uint8_t buffer[64];
  while (tud_midi_available()) {
    uint32_t bytes_to_process = tud_midi_stream_read(buffer, sizeof(buffer));
    uint8_t* ptr = buffer;

    while (bytes_to_process > 0) {
      HandleMIDIMessage(ptr);

      do {
        ptr++;
        bytes_to_process--;
      } while (bytes_to_process > 0 && !(*ptr & 0x80U));
    }
  }
}

MidiState MIDIWorker::Snapshot() const {
  return state_;
}

}  // namespace mtws
