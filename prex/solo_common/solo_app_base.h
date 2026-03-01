#pragma once

#include "ComputerCard.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "prex/mtws/dsp/sine_lut.h"
#include "prex/solo_common/solo_control_router.h"

namespace solo_common {

template <typename EngineT>
class SoloAppBase : public ComputerCard {
 public:
  static constexpr uint32_t kControlDivisor = 16;
  static constexpr uint32_t kControlPeriodUs = (kControlDivisor * 1000000U) / 48000U;

  SoloAppBase()
      : lut_(),
        engine_(&lut_),
        published_ui_index_(0),
        published_frame_index_(0),
        control_counter_(int(kControlDivisor)),
        panel_alt_latched_(false),
        core1_started_(false) {
    instance_ = this;

    engine_.Init();

    UISnapshot ui{};
    ui.knob_main = 2048;
    ui.knob_x = 0;
    ui.knob_y = 0;
    ui.audio1 = 0;
    ui.audio2 = 0;
    ui.audio1_connected = false;
    ui.audio2_connected = false;
    ui.alt = false;

    ui_snapshots_[0] = ui;
    ui_snapshots_[1] = ui;

    ControlFrame c = SoloControlRouter::Build(ui);
    for (int i = 0; i < 2; ++i) {
      engine_.BuildRenderFrame(c, render_frames_[i]);
    }
  }

  static void Core1Entry() {
    if (instance_ != nullptr) {
      instance_->ControlCoreLoop();
    }
  }

  void ControlCoreLoop() {
    while (1) {
      __dmb();
      uint32_t ui_read = published_ui_index_;
      UISnapshot ui = ui_snapshots_[ui_read];

      ControlFrame control = SoloControlRouter::Build(ui);

      uint32_t read_index = published_frame_index_;
      uint32_t write_index = read_index ^ 1U;
      engine_.BuildRenderFrame(control, render_frames_[write_index]);

      __dmb();
      published_frame_index_ = write_index;

      busy_wait_us_32(kControlPeriodUs);
    }
  }

  void CaptureUISnapshot() {
    Switch sw = SwitchVal();
    if (sw == Up) {
      panel_alt_latched_ = true;
    } else if (sw == Middle) {
      panel_alt_latched_ = false;
    }

    UISnapshot ui{};
    int32_t k_main = KnobVal(Knob::Main);
    int32_t k_x = KnobVal(Knob::X);
    int32_t k_y = KnobVal(Knob::Y);

    ui.knob_main = uint16_t(k_main < 0 ? 0 : (k_main > 4095 ? 4095 : k_main));
    ui.knob_x = uint16_t(k_x < 0 ? 0 : (k_x > 4095 ? 4095 : k_x));
    ui.knob_y = uint16_t(k_y < 0 ? 0 : (k_y > 4095 ? 4095 : k_y));

    ui.audio1 = int16_t(AudioIn1());
    ui.audio2 = int16_t(AudioIn2());
    ui.audio1_connected = Connected(Input::Audio1);
    ui.audio2_connected = Connected(Input::Audio2);
    ui.alt = panel_alt_latched_;

    __dmb();
    uint32_t ui_read = published_ui_index_;
    uint32_t ui_write = ui_read ^ 1U;
    ui_snapshots_[ui_write] = ui;
    __dmb();
    published_ui_index_ = ui_write;
  }

  void ProcessSample() override {
    if (!core1_started_) {
      core1_started_ = true;
      multicore_launch_core1(Core1Entry);
    }

    if (++control_counter_ >= int(kControlDivisor)) {
      control_counter_ = 0;
      CaptureUISnapshot();
    }

    __dmb();
    uint32_t read_index = published_frame_index_;
    int32_t out1 = 0;
    int32_t out2 = 0;
    engine_.RenderSample(render_frames_[read_index], out1, out2);

    if (out1 > 2047) out1 = 2047;
    if (out1 < -2048) out1 = -2048;
    if (out2 > 2047) out2 = 2047;
    if (out2 < -2048) out2 = -2048;

    AudioOut1(int16_t(out1));
    AudioOut2(int16_t(out2));
  }

 private:
  static SoloAppBase* instance_;

  mtws::SineLUT lut_;
  EngineT engine_;

  volatile uint32_t published_ui_index_;
  UISnapshot ui_snapshots_[2];

  volatile uint32_t published_frame_index_;
  typename EngineT::RenderFrame render_frames_[2];

  int control_counter_;
  bool panel_alt_latched_;
  bool core1_started_;
};

template <typename EngineT>
SoloAppBase<EngineT>* SoloAppBase<EngineT>::instance_ = nullptr;

}  // namespace solo_common
