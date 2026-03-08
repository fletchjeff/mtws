#include "ComputerCard.h"

#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include <cstdio>
#include <cstdint>

namespace {

// Reports one snapshot every 50 ms so oscillating inputs show useful min/max
// ranges without flooding the USB CDC link.
constexpr uint32_t kReportWindowSamples = 2400U;  // 48000 * 0.05 s

// Holds one USB-reportable view of the board inputs.
// Inputs:
// - written by the ComputerCard audio callback after one report window
// Output:
// - read by the USB serial loop on core 0 and printed as one line
struct InputMonitorSnapshot {
  uint32_t sequence;
  int16_t audio1_current;
  int16_t audio1_min;
  int16_t audio1_max;
  int16_t audio2_current;
  int16_t audio2_min;
  int16_t audio2_max;
  int16_t cv1_current;
  int16_t cv1_min;
  int16_t cv1_max;
  int16_t cv2_current;
  int16_t cv2_min;
  int16_t cv2_max;
  uint16_t pulse1_rising_edges;
  uint16_t pulse2_rising_edges;
  bool pulse1_state;
  bool pulse2_state;
  bool audio1_connected;
  bool audio2_connected;
  bool cv1_connected;
  bool cv2_connected;
  bool pulse1_connected;
  bool pulse2_connected;
};

// Shared double buffer between the audio-processing core and the USB-printing
// core. Core 1 writes the inactive slot, then flips the published index.
volatile uint32_t g_published_snapshot_index = 0;
InputMonitorSnapshot g_snapshots[2] = {};

// Tracks the in-progress min/max and pulse counts for one report window.
// Inputs:
// - updated once per sample inside ProcessSample
// Output:
// - copied into a published snapshot every kReportWindowSamples
struct RunningWindow {
  uint32_t samples_seen;
  uint32_t sequence;
  int16_t audio1_current;
  int16_t audio1_min;
  int16_t audio1_max;
  int16_t audio2_current;
  int16_t audio2_min;
  int16_t audio2_max;
  int16_t cv1_current;
  int16_t cv1_min;
  int16_t cv1_max;
  int16_t cv2_current;
  int16_t cv2_min;
  int16_t cv2_max;
  uint16_t pulse1_rising_edges;
  uint16_t pulse2_rising_edges;
  bool pulse1_state;
  bool pulse2_state;
  bool audio1_connected;
  bool audio2_connected;
  bool cv1_connected;
  bool cv2_connected;
  bool pulse1_connected;
  bool pulse2_connected;
};

// Expands the stored min/max bounds with one new sample.
// Inputs:
// - current minimum and maximum for one channel
// - the new sample value
// Output:
// - the min/max references updated in place
inline void UpdateRange(int16_t value, int16_t& min_value, int16_t& max_value) {
  if (value < min_value) min_value = value;
  if (value > max_value) max_value = value;
}

// Resets one running report window so the next 50 ms block starts with the
// current sample values as both endpoints.
// Inputs:
// - the running window object to clear
// Output:
// - the window is ready to accumulate a fresh min/max interval
void ResetWindow(RunningWindow& window) {
  window.samples_seen = 0;
  window.audio1_current = 0;
  window.audio1_min = 2047;
  window.audio1_max = -2048;
  window.audio2_current = 0;
  window.audio2_min = 2047;
  window.audio2_max = -2048;
  window.cv1_current = 0;
  window.cv1_min = 2047;
  window.cv1_max = -2048;
  window.cv2_current = 0;
  window.cv2_min = 2047;
  window.cv2_max = -2048;
  window.pulse1_rising_edges = 0;
  window.pulse2_rising_edges = 0;
  window.pulse1_state = false;
  window.pulse2_state = false;
  window.audio1_connected = false;
  window.audio2_connected = false;
  window.cv1_connected = false;
  window.cv2_connected = false;
  window.pulse1_connected = false;
  window.pulse2_connected = false;
}

// Prints the fixed tab-separated column header once at startup.
// Inputs:
// - none
// Output:
// - one header line sent over USB CDC stdout
void PrintHeader() {
  printf(
      "seq\t"
      "a1\ta1_min\ta1_max\t"
      "a2\ta2_min\ta2_max\t"
      "cv1\tcv1_min\tcv1_max\t"
      "cv2\tcv2_min\tcv2_max\t"
      "p1\tp1_edges\tp2\tp2_edges\t"
      "c_a1\tc_a2\tc_cv1\tc_cv2\tc_p1\tc_p2\n");
}

// Prints one published snapshot in a compact tab-separated format that is easy
// to watch in a terminal or copy into a spreadsheet.
// Inputs:
// - the published snapshot to display
// Output:
// - one data line sent over USB CDC stdout
void PrintSnapshot(const InputMonitorSnapshot& snapshot) {
  printf(
      "%lu\t"
      "%d\t%d\t%d\t"
      "%d\t%d\t%d\t"
      "%d\t%d\t%d\t"
      "%d\t%d\t%d\t"
      "%u\t%u\t%u\t%u\t"
      "%u\t%u\t%u\t%u\t%u\t%u\n",
      static_cast<unsigned long>(snapshot.sequence),
      snapshot.audio1_current,
      snapshot.audio1_min,
      snapshot.audio1_max,
      snapshot.audio2_current,
      snapshot.audio2_min,
      snapshot.audio2_max,
      snapshot.cv1_current,
      snapshot.cv1_min,
      snapshot.cv1_max,
      snapshot.cv2_current,
      snapshot.cv2_min,
      snapshot.cv2_max,
      static_cast<unsigned>(snapshot.pulse1_state),
      static_cast<unsigned>(snapshot.pulse1_rising_edges),
      static_cast<unsigned>(snapshot.pulse2_state),
      static_cast<unsigned>(snapshot.pulse2_rising_edges),
      static_cast<unsigned>(snapshot.audio1_connected),
      static_cast<unsigned>(snapshot.audio2_connected),
      static_cast<unsigned>(snapshot.cv1_connected),
      static_cast<unsigned>(snapshot.cv2_connected),
      static_cast<unsigned>(snapshot.pulse1_connected),
      static_cast<unsigned>(snapshot.pulse2_connected));
}

class InputMonitorApp : public ComputerCard {
 public:
  // Creates the monitor app and enables jack-detect probing before Run().
  // Inputs:
  // - none
  // Output:
  // - app ready to sample all six input jacks
  InputMonitorApp() : window_{} {
    EnableNormalisationProbe();
    ResetWindow(window_);
  }

  // Samples the current analogue and pulse inputs once per audio frame,
  // accumulates min/max bounds for the current report window, and publishes one
  // USB snapshot every 2400 samples.
  // Inputs:
  // - implicit current board input states via ComputerCard helpers
  // Output:
  // - shared snapshot buffer updated every 50 ms
  void ProcessSample() override {
    window_.audio1_current = AudioIn1();
    window_.audio2_current = AudioIn2();
    window_.cv1_current = CVIn1();
    window_.cv2_current = CVIn2();

    UpdateRange(window_.audio1_current, window_.audio1_min, window_.audio1_max);
    UpdateRange(window_.audio2_current, window_.audio2_min, window_.audio2_max);
    UpdateRange(window_.cv1_current, window_.cv1_min, window_.cv1_max);
    UpdateRange(window_.cv2_current, window_.cv2_min, window_.cv2_max);

    window_.pulse1_state = PulseIn1();
    window_.pulse2_state = PulseIn2();
    if (PulseIn1RisingEdge()) ++window_.pulse1_rising_edges;
    if (PulseIn2RisingEdge()) ++window_.pulse2_rising_edges;

    window_.audio1_connected = Connected(Input::Audio1);
    window_.audio2_connected = Connected(Input::Audio2);
    window_.cv1_connected = Connected(Input::CV1);
    window_.cv2_connected = Connected(Input::CV2);
    window_.pulse1_connected = Connected(Input::Pulse1);
    window_.pulse2_connected = Connected(Input::Pulse2);

    ++window_.samples_seen;
    if (window_.samples_seen < kReportWindowSamples) {
      return;
    }

    InputMonitorSnapshot snapshot{};
    snapshot.sequence = ++window_.sequence;
    snapshot.audio1_current = window_.audio1_current;
    snapshot.audio1_min = window_.audio1_min;
    snapshot.audio1_max = window_.audio1_max;
    snapshot.audio2_current = window_.audio2_current;
    snapshot.audio2_min = window_.audio2_min;
    snapshot.audio2_max = window_.audio2_max;
    snapshot.cv1_current = window_.cv1_current;
    snapshot.cv1_min = window_.cv1_min;
    snapshot.cv1_max = window_.cv1_max;
    snapshot.cv2_current = window_.cv2_current;
    snapshot.cv2_min = window_.cv2_min;
    snapshot.cv2_max = window_.cv2_max;
    snapshot.pulse1_rising_edges = window_.pulse1_rising_edges;
    snapshot.pulse2_rising_edges = window_.pulse2_rising_edges;
    snapshot.pulse1_state = window_.pulse1_state;
    snapshot.pulse2_state = window_.pulse2_state;
    snapshot.audio1_connected = window_.audio1_connected;
    snapshot.audio2_connected = window_.audio2_connected;
    snapshot.cv1_connected = window_.cv1_connected;
    snapshot.cv2_connected = window_.cv2_connected;
    snapshot.pulse1_connected = window_.pulse1_connected;
    snapshot.pulse2_connected = window_.pulse2_connected;

    uint32_t read_index = g_published_snapshot_index;
    uint32_t write_index = read_index ^ 1U;
    g_snapshots[write_index] = snapshot;
    __dmb();
    g_published_snapshot_index = write_index;

    ResetWindow(window_);
    window_.sequence = snapshot.sequence;
  }

 private:
  RunningWindow window_;
};

// Runs the ComputerCard sampler on core 1 so the RP2040 USB stdio machinery
// can remain on core 0.
// Inputs:
// - none
// Output:
// - never returns; starts the 48 kHz sampling loop on core 1
void Core1Main() {
  InputMonitorApp app;
  app.Run();
}

}  // namespace

// Initializes USB CDC stdout, launches the ComputerCard sampler on core 1, and
// prints each published snapshot as it arrives.
// Inputs:
// - none
// Output:
// - never returns; continuously streams input readings over USB serial
int main() {
  sleep_ms(500);
  stdio_init_all();
  sleep_ms(500);

  PrintHeader();
  multicore_launch_core1(Core1Main);

  uint32_t last_sequence = 0;
  while (1) {
    __dmb();
    uint32_t read_index = g_published_snapshot_index;
    InputMonitorSnapshot snapshot = g_snapshots[read_index];
    if (snapshot.sequence != 0U && snapshot.sequence != last_sequence) {
      PrintSnapshot(snapshot);
      last_sequence = snapshot.sequence;
    }
    sleep_ms(5);
  }
}
