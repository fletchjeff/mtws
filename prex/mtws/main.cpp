#include "ComputerCard.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "prex/mtws/core/control_router.h"
#include "prex/mtws/core/midi_worker.h"
#include "prex/mtws/core/shared_state.h"
#include "prex/mtws/dsp/sine_lut.h"
#include "prex/mtws/engines/engine_registry.h"

namespace mtws {

class MTWSApp : public ComputerCard {
 public:
  static constexpr uint32_t kControlDivisor = 16;
  static constexpr uint32_t kControlPeriodUs = (kControlDivisor * 1000000U) / 48000U;
  static constexpr uint8_t kSwitchDebounceTicks = 3;
  static constexpr uint32_t kCrossfadeSamples = 64;  // ~1.33ms
  static constexpr uint32_t kNoteFlashOffSamples = 960;  // ~20ms

  MTWSApp()
      : lut_(),
        registry_(&lut_),
        midi_worker_(),
        published_ui_snapshot_index_(0),
        published_frame_index_(0),
        control_counter_(int(kControlDivisor)),
        switch_candidate_(Middle),
        switch_stable_(Middle),
        switch_stable_count_(0),
        panel_alt_latched_(false),
        selected_slot_(5),
        previous_slot_(0),
        transition_samples_remaining_(0),
        seen_note_on_counter_(0),
        note_flash_samples_remaining_(0),
        last_cv_note_sent_(255),
        core1_started_(false) {
    instance_ = this;

    EnableNormalisationProbe();
    registry_.Init();
    registry_.Get(selected_slot_)->OnSelected();

    UISnapshot init_ui{};
    init_ui.knob_main = 2048;
    init_ui.knob_x = 0;
    init_ui.knob_y = 0;
    init_ui.cv1 = 0;
    init_ui.cv2 = 0;
    init_ui.audio1 = 0;
    init_ui.audio2 = 0;
    init_ui.audio1_connected = false;
    init_ui.audio2_connected = false;
    init_ui.cv2_connected = false;
    init_ui.pulse1_connected = false;
    init_ui.pulse1_high = false;
    init_ui.selected_slot = selected_slot_;
    init_ui.panel_alt_latched = panel_alt_latched_;
    ui_snapshots_[0] = init_ui;
    ui_snapshots_[1] = init_ui;

    MidiState init_midi{};
    init_midi.note_active = false;
    init_midi.current_note = 60;
    init_midi.last_note = 60;
    init_midi.cc1 = 0;
    init_midi.cc74 = 0;
    init_midi.note_on_counter = 0;

    GlobalControlFrame init_global = ControlRouter::BuildGlobalFrame(init_ui, init_midi);
    for (int i = 0; i < 2; ++i) {
      control_frames_[i].global = init_global;
      registry_.Get(init_global.selected_slot)->ControlTick(init_global, control_frames_[i].engine);
    }
    transition_frame_ = control_frames_[0];
  }

  static void Core1Entry() {
    if (instance_ != nullptr) {
      instance_->ControlAndMIDIWorkerCore();
    }
  }

  void ControlAndMIDIWorkerCore() {
    midi_worker_.Init();

    while (1) {
      midi_worker_.Poll();

      __dmb();
      uint32_t ui_read_index = published_ui_snapshot_index_;
      UISnapshot ui = ui_snapshots_[ui_read_index];
      MidiState midi = midi_worker_.Snapshot();

      GlobalControlFrame global = ControlRouter::BuildGlobalFrame(ui, midi);

      uint32_t read_index = published_frame_index_;
      uint32_t write_index = read_index ^ 1U;
      ControlFrame& frame = control_frames_[write_index];
      frame.global = global;
      registry_.Get(global.selected_slot)->ControlTick(global, frame.engine);

      __dmb();
      published_frame_index_ = write_index;

      busy_wait_us_32(kControlPeriodUs);
    }
  }

  inline void CaptureUISnapshotAndHandleSwitching() {
    Switch prev_stable = switch_stable_;
    Switch sw = SwitchVal();
    if (sw != switch_candidate_) {
      switch_candidate_ = sw;
      switch_stable_count_ = 0;
    } else if (switch_candidate_ != switch_stable_) {
      if (switch_stable_count_ < kSwitchDebounceTicks) {
        ++switch_stable_count_;
      }
      if (switch_stable_count_ >= kSwitchDebounceTicks) {
        switch_stable_ = switch_candidate_;
        switch_stable_count_ = 0;
      }
    } else {
      switch_stable_count_ = 0;
    }
    sw = switch_stable_;

    if (sw == Up) {
      panel_alt_latched_ = true;
    } else if (sw == Middle) {
      panel_alt_latched_ = false;
    }

    bool down_edge = (sw == Down) && (prev_stable != Down);
    if (down_edge) {
      previous_slot_ = selected_slot_;
      selected_slot_ = uint8_t((selected_slot_ + 1U) % kNumOscillatorSlots);

      uint32_t read_index = published_frame_index_;
      __dmb();
      transition_frame_ = control_frames_[read_index];
      transition_samples_remaining_ = int(kCrossfadeSamples);

      registry_.Get(selected_slot_)->OnSelected();
    }

    UISnapshot ui{};
    ui.knob_main = uint16_t(KnobVal(Knob::Main) < 0 ? 0 : (KnobVal(Knob::Main) > 4095 ? 4095 : KnobVal(Knob::Main)));
    ui.knob_x = uint16_t(KnobVal(Knob::X) < 0 ? 0 : (KnobVal(Knob::X) > 4095 ? 4095 : KnobVal(Knob::X)));
    ui.knob_y = uint16_t(KnobVal(Knob::Y) < 0 ? 0 : (KnobVal(Knob::Y) > 4095 ? 4095 : KnobVal(Knob::Y)));

    ui.cv1 = int16_t(CVIn1());
    ui.cv2 = int16_t(CVIn2());
    ui.audio1 = int16_t(AudioIn1());
    ui.audio2 = int16_t(AudioIn2());

    ui.audio1_connected = Connected(Input::Audio1);
    ui.audio2_connected = Connected(Input::Audio2);
    ui.cv2_connected = Connected(Input::CV2);
    ui.pulse1_connected = Connected(Input::Pulse1);
    ui.pulse1_high = PulseIn1();

    ui.selected_slot = selected_slot_;
    ui.panel_alt_latched = panel_alt_latched_;

    __dmb();
    uint32_t ui_read_index = published_ui_snapshot_index_;
    uint32_t ui_write_index = ui_read_index ^ 1U;
    ui_snapshots_[ui_write_index] = ui;
    __dmb();
    published_ui_snapshot_index_ = ui_write_index;
  }

  inline void UpdateSlotLEDs(const ControlFrame& frame) {
    if (frame.global.note_on_counter != seen_note_on_counter_) {
      seen_note_on_counter_ = frame.global.note_on_counter;
      note_flash_samples_remaining_ = int(kNoteFlashOffSamples);
    }

    bool flash_off = note_flash_samples_remaining_ > 0;
    for (uint32_t i = 0; i < 6; ++i) {
      bool selected = (i == frame.global.selected_slot);
      LedOn(i, selected && !flash_off);
    }
  }

  virtual void ProcessSample() override {
    if (!core1_started_) {
      core1_started_ = true;
      multicore_launch_core1(Core1Entry);
    }

    if (++control_counter_ >= int(kControlDivisor)) {
      control_counter_ = 0;
      CaptureUISnapshotAndHandleSwitching();

      uint32_t read_index = published_frame_index_;
      __dmb();
      UpdateSlotLEDs(control_frames_[read_index]);
    }

    if (note_flash_samples_remaining_ > 0) {
      --note_flash_samples_remaining_;
    }

    uint32_t read_index = published_frame_index_;
    __dmb();
    const ControlFrame& frame = control_frames_[read_index];

    int32_t new_out1 = 0;
    int32_t new_out2 = 0;
    registry_.Get(frame.global.selected_slot)->RenderSample(frame.engine, new_out1, new_out2);

    int32_t mixed_out1 = new_out1;
    int32_t mixed_out2 = new_out2;

    if (transition_samples_remaining_ > 0) {
      int32_t old_out1 = 0;
      int32_t old_out2 = 0;
      registry_.Get(previous_slot_)->RenderSample(transition_frame_.engine, old_out1, old_out2);

      uint32_t fade_in_q12 = uint32_t((int64_t(kCrossfadeSamples - uint32_t(transition_samples_remaining_)) * 4096) / kCrossfadeSamples);
      if (fade_in_q12 > 4096U) fade_in_q12 = 4096U;
      uint32_t fade_out_q12 = 4096U - fade_in_q12;

      mixed_out1 = int32_t((int64_t(old_out1) * fade_out_q12 + int64_t(new_out1) * fade_in_q12) >> 12);
      mixed_out2 = int32_t((int64_t(old_out2) * fade_out_q12 + int64_t(new_out2) * fade_in_q12) >> 12);

      --transition_samples_remaining_;
    }

    int32_t out1 = int32_t((int64_t(mixed_out1) * frame.global.vca_gain_q12) >> 12);
    int32_t out2 = int32_t((int64_t(mixed_out2) * frame.global.vca_gain_q12) >> 12);

    if (out1 > 2047) out1 = 2047;
    if (out1 < -2048) out1 = -2048;
    if (out2 > 2047) out2 = 2047;
    if (out2 < -2048) out2 = -2048;

    AudioOut1(int16_t(out1));
    AudioOut2(int16_t(out2));

    if (frame.global.last_midi_note != last_cv_note_sent_) {
      CVOut1MIDINote(frame.global.last_midi_note);
      last_cv_note_sent_ = frame.global.last_midi_note;
    }
  }

 private:
  static MTWSApp* instance_;

  SineLUT lut_;
  EngineRegistry registry_;
  MIDIWorker midi_worker_;

  volatile uint32_t published_ui_snapshot_index_;
  UISnapshot ui_snapshots_[2];
  volatile uint32_t published_frame_index_;
  ControlFrame control_frames_[2];

  int control_counter_;
  Switch switch_candidate_;
  Switch switch_stable_;
  uint8_t switch_stable_count_;
  bool panel_alt_latched_;
  uint8_t selected_slot_;

  uint8_t previous_slot_;
  int transition_samples_remaining_;
  ControlFrame transition_frame_;

  uint32_t seen_note_on_counter_;
  int note_flash_samples_remaining_;
  uint8_t last_cv_note_sent_;

  bool core1_started_;
};

MTWSApp* MTWSApp::instance_ = nullptr;

}  // namespace mtws

int main() {
  vreg_set_voltage(VREG_VOLTAGE_1_15);
  sleep_ms(10);
  set_sys_clock_khz(200000, true);

  mtws::MTWSApp app;
  app.Run();
}
