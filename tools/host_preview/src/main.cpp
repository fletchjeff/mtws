#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#endif

#include "prex/bender/bender_solo_engine.h"
#include "prex/cumulus/cumulus_solo_engine.h"
#include "prex/din_sum/din_sum_solo_engine.h"
#include "prex/floatable/floatable_solo_engine.h"
#include "prex/losenge/losenge_solo_engine.h"
#include "prex/sawsome/sawsome_solo_engine.h"
#include "prex/solo_common/solo_control_router.h"

namespace {

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kControlDivisor = 16;

enum class EngineKind {
  kSawsome,
  kBender,
  kFloatable,
  kCumulus,
  kLosenge,
  kDinSum,
};

struct CliOptions {
  EngineKind engine = EngineKind::kSawsome;
  double seconds = 8.0;
  bool seconds_set = false;
  std::string out_path = "preview.wav";
  std::string script_path;

  bool realtime = false;
  double duration = -1.0;
  bool duration_set = false;

  int knob_main = 2048;
  int knob_x = 0;
  int knob_y = 0;
  int audio1 = 0;
  int audio2 = 0;
  bool audio1_connected = false;
  bool audio2_connected = false;
  bool alt = false;

  bool help_requested = false;
};

struct ControlState {
  int knob_main = 2048;
  int knob_x = 0;
  int knob_y = 0;
  int audio1 = 0;
  int audio2 = 0;
  bool audio1_connected = false;
  bool audio2_connected = false;
  bool alt = false;
};

struct AutomationEvent {
  uint64_t sample_index = 0;
  ControlState state;
};

std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
  return s;
}

std::string Trim(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::vector<std::string> SplitCsv(const std::string& line) {
  std::vector<std::string> out;
  std::string current;
  for (char c : line) {
    if (c == ',') {
      out.push_back(Trim(current));
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  out.push_back(Trim(current));
  return out;
}

bool ParseBool(const std::string& text, bool& out) {
  std::string s = ToLower(Trim(text));
  if (s == "1" || s == "true" || s == "yes" || s == "on") {
    out = true;
    return true;
  }
  if (s == "0" || s == "false" || s == "no" || s == "off") {
    out = false;
    return true;
  }
  return false;
}

bool ParseInt(const std::string& text, int& out) {
  try {
    std::string t = Trim(text);
    size_t used = 0;
    int v = std::stoi(t, &used, 10);
    if (used != t.size()) return false;
    out = v;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseDouble(const std::string& text, double& out) {
  try {
    std::string t = Trim(text);
    size_t used = 0;
    double v = std::stod(t, &used);
    if (used != t.size()) return false;
    out = v;
    return true;
  } catch (...) {
    return false;
  }
}

uint16_t ClampU12(int v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return static_cast<uint16_t>(v);
}

int16_t ClampIn12(int v) {
  if (v < -2048) return -2048;
  if (v > 2047) return 2047;
  return static_cast<int16_t>(v);
}

int32_t ClampAudio12(int32_t v) {
  if (v < -2048) return -2048;
  if (v > 2047) return 2047;
  return v;
}

void PrintUsage(const char* exe) {
  std::cout
      << "Usage: " << exe << " [options]\n"
      << "  --engine <sawsome|bender|floatable|cumulus|losenge|din_sum>\n"
      << "  --seconds <float>            (offline WAV render only)\n"
      << "  --out <path.wav>             (offline WAV render only)\n"
      << "  --realtime                   (stream to Mac default output)\n"
      << "  --duration <float>           (seconds for realtime; omit for run-until-Ctrl+C)\n"
      << "  --main <0..4095>\n"
      << "  --x <0..4095>\n"
      << "  --y <0..4095>\n"
      << "  --audio1 <-2048..2047>\n"
      << "  --audio2 <-2048..2047>\n"
      << "  --audio1-connected <0|1>\n"
      << "  --audio2-connected <0|1>\n"
      << "  --alt <0|1>\n"
      << "  --script <automation.csv>\n"
      << "\n"
      << "CSV automation columns (header row required):\n"
      << "  time_s,time_ms,main,x,y,alt,audio1,audio2,audio1_connected,audio2_connected\n"
      << "Example:\n"
      << "  time_s,main,x,y,alt\n"
      << "  0.0,2048,0,0,0\n"
      << "  1.0,2048,4095,0,0\n"
      << "  2.0,2048,4095,4095,1\n";
}

bool ParseEngine(const std::string& text, EngineKind& out) {
  const std::string v = ToLower(text);
  if (v == "sawsome") {
    out = EngineKind::kSawsome;
    return true;
  }
  if (v == "bender") {
    out = EngineKind::kBender;
    return true;
  }
  if (v == "floatable") {
    out = EngineKind::kFloatable;
    return true;
  }
  if (v == "cumulus") {
    out = EngineKind::kCumulus;
    return true;
  }
  if (v == "losenge") {
    out = EngineKind::kLosenge;
    return true;
  }
  if (v == "din_sum") {
    out = EngineKind::kDinSum;
    return true;
  }
  return false;
}

const char* EngineName(EngineKind kind) {
  switch (kind) {
    case EngineKind::kSawsome:
      return "sawsome";
    case EngineKind::kBender:
      return "bender";
    case EngineKind::kFloatable:
      return "floatable";
    case EngineKind::kCumulus:
      return "cumulus";
    case EngineKind::kLosenge:
      return "losenge";
    case EngineKind::kDinSum:
      return "din_sum";
  }
  return "unknown";
}

bool ParseCli(int argc, char** argv, CliOptions& out) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto require_value = [&](const char* name) -> const char* {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        return nullptr;
      }
      return argv[++i];
    };

    if (arg == "--help" || arg == "-h") {
      out.help_requested = true;
      return false;
    }

    if (arg == "--realtime") {
      out.realtime = true;
      continue;
    }

    if (arg == "--engine") {
      const char* v = require_value("--engine");
      if (!v) return false;
      if (!ParseEngine(v, out.engine)) {
        std::cerr << "Invalid engine: " << v << "\n";
        return false;
      }
      continue;
    }

    if (arg == "--seconds") {
      const char* v = require_value("--seconds");
      if (!v) return false;
      if (!ParseDouble(v, out.seconds) || out.seconds <= 0.0) {
        std::cerr << "Invalid --seconds value\n";
        return false;
      }
      out.seconds_set = true;
      continue;
    }

    if (arg == "--duration") {
      const char* v = require_value("--duration");
      if (!v) return false;
      if (!ParseDouble(v, out.duration) || out.duration <= 0.0) {
        std::cerr << "Invalid --duration value\n";
        return false;
      }
      out.duration_set = true;
      continue;
    }

    if (arg == "--out") {
      const char* v = require_value("--out");
      if (!v) return false;
      out.out_path = v;
      continue;
    }

    if (arg == "--script") {
      const char* v = require_value("--script");
      if (!v) return false;
      out.script_path = v;
      continue;
    }

    auto parse_int_option = [&](const char* name, int& target) -> bool {
      const char* v = require_value(name);
      if (!v) return false;
      if (!ParseInt(v, target)) {
        std::cerr << "Invalid value for " << name << "\n";
        return false;
      }
      return true;
    };

    if (arg == "--main") {
      if (!parse_int_option("--main", out.knob_main)) return false;
      continue;
    }
    if (arg == "--x") {
      if (!parse_int_option("--x", out.knob_x)) return false;
      continue;
    }
    if (arg == "--y") {
      if (!parse_int_option("--y", out.knob_y)) return false;
      continue;
    }
    if (arg == "--audio1") {
      if (!parse_int_option("--audio1", out.audio1)) return false;
      continue;
    }
    if (arg == "--audio2") {
      if (!parse_int_option("--audio2", out.audio2)) return false;
      continue;
    }

    if (arg == "--alt" || arg == "--audio1-connected" || arg == "--audio2-connected") {
      const char* v = require_value(arg.c_str());
      if (!v) return false;
      bool parsed = false;
      if (!ParseBool(v, parsed)) {
        std::cerr << "Invalid boolean for " << arg << "\n";
        return false;
      }
      if (arg == "--alt") out.alt = parsed;
      if (arg == "--audio1-connected") out.audio1_connected = parsed;
      if (arg == "--audio2-connected") out.audio2_connected = parsed;
      continue;
    }

    std::cerr << "Unknown argument: " << arg << "\n";
    return false;
  }

  return true;
}

class IEngineRunner {
 public:
  virtual ~IEngineRunner() = default;
  virtual void UpdateFrame(const solo_common::ControlFrame& frame) = 0;
  virtual void RenderSample(int32_t& out1, int32_t& out2) = 0;
};

template <typename EngineT>
class EngineRunner final : public IEngineRunner {
 public:
  EngineRunner() : lut_(), engine_(&lut_), frame_{} {
    engine_.Init();
    solo_common::ControlFrame init{};
    init.pitch_inc = 894785U;
    init.macro_x = 0;
    init.macro_y = 0;
    init.alt = false;
    engine_.BuildRenderFrame(init, frame_);
  }

  void UpdateFrame(const solo_common::ControlFrame& frame) override { engine_.BuildRenderFrame(frame, frame_); }

  void RenderSample(int32_t& out1, int32_t& out2) override { engine_.RenderSample(frame_, out1, out2); }

 private:
  mtws::SineLUT lut_;
  EngineT engine_;
  typename EngineT::RenderFrame frame_;
};

std::unique_ptr<IEngineRunner> CreateRunner(EngineKind kind) {
  switch (kind) {
    case EngineKind::kSawsome:
      return std::make_unique<EngineRunner<SawsomeSoloEngine>>();
    case EngineKind::kBender:
      return std::make_unique<EngineRunner<BenderSoloEngine>>();
    case EngineKind::kFloatable:
      return std::make_unique<EngineRunner<FloatableSoloEngine>>();
    case EngineKind::kCumulus:
      return std::make_unique<EngineRunner<CumulusSoloEngine>>();
    case EngineKind::kLosenge:
      return std::make_unique<EngineRunner<LosengeSoloEngine>>();
    case EngineKind::kDinSum:
      return std::make_unique<EngineRunner<DinSumSoloEngine>>();
  }
  return nullptr;
}

void WriteLE16(std::ofstream& f, uint16_t v) {
  char b[2] = {static_cast<char>(v & 0xFFU), static_cast<char>((v >> 8) & 0xFFU)};
  f.write(b, 2);
}

void WriteLE32(std::ofstream& f, uint32_t v) {
  char b[4] = {
      static_cast<char>(v & 0xFFU),
      static_cast<char>((v >> 8) & 0xFFU),
      static_cast<char>((v >> 16) & 0xFFU),
      static_cast<char>((v >> 24) & 0xFFU),
  };
  f.write(b, 4);
}

class WavWriter {
 public:
  bool Open(const std::string& path, uint32_t sample_rate) {
    file_.open(path, std::ios::binary);
    if (!file_.is_open()) {
      std::cerr << "Failed to open output wav: " << path << "\n";
      return false;
    }

    file_.write("RIFF", 4);
    WriteLE32(file_, 0);
    file_.write("WAVE", 4);

    file_.write("fmt ", 4);
    WriteLE32(file_, 16);
    WriteLE16(file_, 1);
    WriteLE16(file_, 2);
    WriteLE32(file_, sample_rate);
    WriteLE32(file_, sample_rate * 4);
    WriteLE16(file_, 4);
    WriteLE16(file_, 16);

    file_.write("data", 4);
    WriteLE32(file_, 0);

    return true;
  }

  void WriteFrame(int16_t left, int16_t right) {
    if (!file_.is_open()) return;
    WriteLE16(file_, static_cast<uint16_t>(left));
    WriteLE16(file_, static_cast<uint16_t>(right));
    ++frames_written_;
  }

  bool Close() {
    if (!file_.is_open()) return true;

    const uint32_t data_size = frames_written_ * 4;
    const uint32_t riff_size = 36 + data_size;

    file_.seekp(4, std::ios::beg);
    WriteLE32(file_, riff_size);

    file_.seekp(40, std::ios::beg);
    WriteLE32(file_, data_size);

    file_.close();
    return true;
  }

  ~WavWriter() { Close(); }

 private:
  std::ofstream file_;
  uint32_t frames_written_ = 0;
};

bool LooksLikeHeaderRow(const std::vector<std::string>& row) {
  for (const std::string& cell : row) {
    for (char c : cell) {
      if (std::isalpha(static_cast<unsigned char>(c))) return true;
    }
  }
  return false;
}

bool LoadAutomationCsv(const std::string& path, const ControlState& initial, std::vector<AutomationEvent>& out_events) {
  std::ifstream in(path);
  if (!in.is_open()) {
    std::cerr << "Failed to open automation CSV: " << path << "\n";
    return false;
  }

  std::string line;
  std::vector<std::string> headers;
  std::unordered_map<std::string, size_t> header_idx;

  while (std::getline(in, line)) {
    if (!Trim(line).empty()) {
      headers = SplitCsv(line);
      break;
    }
  }

  if (headers.empty()) return true;

  bool has_header = LooksLikeHeaderRow(headers);
  if (has_header) {
    for (size_t i = 0; i < headers.size(); ++i) {
      header_idx[ToLower(headers[i])] = i;
    }
  } else {
    header_idx["time_s"] = 0;
    header_idx["main"] = 1;
    header_idx["x"] = 2;
    header_idx["y"] = 3;
    header_idx["alt"] = 4;
  }

  auto parse_row = [&](const std::vector<std::string>& row, ControlState& state_out, uint64_t& sample_out) -> bool {
    ControlState state = out_events.empty() ? initial : out_events.back().state;

    auto get = [&](const std::string& key) -> const std::string* {
      auto it = header_idx.find(key);
      if (it == header_idx.end()) return nullptr;
      if (it->second >= row.size()) return nullptr;
      return &row[it->second];
    };

    double time_s = 0.0;
    if (const std::string* t = get("time_s")) {
      if (!ParseDouble(*t, time_s)) return false;
    } else if (const std::string* tms = get("time_ms")) {
      double ms = 0.0;
      if (!ParseDouble(*tms, ms)) return false;
      time_s = ms * 0.001;
    } else {
      return false;
    }

    auto parse_int_field = [&](const std::string& key, int& target) -> bool {
      const std::string* cell = get(key);
      if (!cell) return true;
      return ParseInt(*cell, target);
    };

    auto parse_bool_field = [&](const std::string& key, bool& target) -> bool {
      const std::string* cell = get(key);
      if (!cell) return true;
      return ParseBool(*cell, target);
    };

    if (!parse_int_field("main", state.knob_main)) return false;
    if (!parse_int_field("x", state.knob_x)) return false;
    if (!parse_int_field("y", state.knob_y)) return false;
    if (!parse_int_field("audio1", state.audio1)) return false;
    if (!parse_int_field("audio2", state.audio2)) return false;
    if (!parse_bool_field("alt", state.alt)) return false;
    if (!parse_bool_field("audio1_connected", state.audio1_connected)) return false;
    if (!parse_bool_field("audio2_connected", state.audio2_connected)) return false;

    if (time_s < 0.0) time_s = 0.0;
    sample_out = static_cast<uint64_t>(std::llround(time_s * static_cast<double>(kSampleRate)));
    state_out = state;
    return true;
  };

  if (!has_header) {
    ControlState row_state;
    uint64_t row_sample = 0;
    if (!parse_row(headers, row_state, row_sample)) {
      std::cerr << "Failed parsing first automation row\n";
      return false;
    }
    out_events.push_back({row_sample, row_state});
  }

  while (std::getline(in, line)) {
    std::string trimmed = Trim(line);
    if (trimmed.empty()) continue;
    if (!trimmed.empty() && trimmed[0] == '#') continue;

    std::vector<std::string> row = SplitCsv(line);
    ControlState row_state;
    uint64_t row_sample = 0;
    if (!parse_row(row, row_state, row_sample)) {
      std::cerr << "Failed parsing automation row: " << line << "\n";
      return false;
    }
    out_events.push_back({row_sample, row_state});
  }

  std::stable_sort(out_events.begin(), out_events.end(), [](const AutomationEvent& a, const AutomationEvent& b) {
    return a.sample_index < b.sample_index;
  });

  return true;
}

class RenderStepper {
 public:
  RenderStepper(IEngineRunner* runner, const ControlState& initial, std::vector<AutomationEvent> events)
      : runner_(runner), state_(initial), events_(std::move(events)) {}

  void Step(int16_t& left, int16_t& right) {
    if ((sample_index_ % kControlDivisor) == 0) {
      while (event_index_ < events_.size() && events_[event_index_].sample_index <= sample_index_) {
        state_ = events_[event_index_].state;
        ++event_index_;
      }
      ApplyControl();
    }

    int32_t out1 = 0;
    int32_t out2 = 0;
    runner_->RenderSample(out1, out2);

    out1 = ClampAudio12(out1);
    out2 = ClampAudio12(out2);

    // Match board 12-bit nominal range to 16-bit output.
    left = static_cast<int16_t>(out1 << 4);
    right = static_cast<int16_t>(out2 << 4);

    ++sample_index_;
  }

  uint64_t SampleIndex() const { return sample_index_; }

 private:
  void ApplyControl() {
    solo_common::UISnapshot ui{};
    ui.knob_main = ClampU12(state_.knob_main);
    ui.knob_x = ClampU12(state_.knob_x);
    ui.knob_y = ClampU12(state_.knob_y);
    ui.audio1 = ClampIn12(state_.audio1);
    ui.audio2 = ClampIn12(state_.audio2);
    ui.audio1_connected = state_.audio1_connected;
    ui.audio2_connected = state_.audio2_connected;
    ui.alt = state_.alt;

    solo_common::ControlFrame frame = solo_common::SoloControlRouter::Build(ui);
    runner_->UpdateFrame(frame);
  }

  IEngineRunner* runner_;
  ControlState state_;
  std::vector<AutomationEvent> events_;
  size_t event_index_ = 0;
  uint64_t sample_index_ = 0;
};

bool RenderOffline(IEngineRunner* runner,
                   const ControlState& initial,
                   const std::vector<AutomationEvent>& events,
                   const std::string& out_path,
                   double seconds,
                   EngineKind engine) {
  WavWriter writer;
  if (!writer.Open(out_path, kSampleRate)) {
    return false;
  }

  RenderStepper stepper(runner, initial, events);
  const uint64_t total_samples = static_cast<uint64_t>(std::llround(seconds * static_cast<double>(kSampleRate)));

  for (uint64_t i = 0; i < total_samples; ++i) {
    int16_t l = 0;
    int16_t r = 0;
    stepper.Step(l, r);
    writer.WriteFrame(l, r);
  }

  writer.Close();

  std::cout << "Rendered " << seconds << "s"
            << " engine=" << EngineName(engine)
            << " samples=" << total_samples
            << " output=" << out_path << "\n";
  return true;
}

#ifdef __APPLE__
std::atomic<bool> g_stop_requested(false);

void OnSigInt(int) {
  g_stop_requested.store(true, std::memory_order_relaxed);
}

struct RealtimeContext {
  RenderStepper stepper;
  uint64_t total_samples;
  std::atomic<bool> finished;

  RealtimeContext(IEngineRunner* runner,
                  const ControlState& initial,
                  const std::vector<AutomationEvent>& events,
                  uint64_t total)
      : stepper(runner, initial, events), total_samples(total), finished(false) {}
};

void AudioQueueCallback(void* user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  auto* ctx = static_cast<RealtimeContext*>(user_data);

  const uint32_t frames = buffer->mAudioDataBytesCapacity / (sizeof(int16_t) * 2);
  auto* out = static_cast<int16_t*>(buffer->mAudioData);

  for (uint32_t i = 0; i < frames; ++i) {
    bool should_stop = g_stop_requested.load(std::memory_order_relaxed);
    if (!should_stop && ctx->total_samples > 0 && ctx->stepper.SampleIndex() >= ctx->total_samples) {
      should_stop = true;
    }

    if (should_stop) {
      out[(i << 1)] = 0;
      out[(i << 1) + 1] = 0;
      ctx->finished.store(true, std::memory_order_release);
      continue;
    }

    int16_t l = 0;
    int16_t r = 0;
    ctx->stepper.Step(l, r);
    out[(i << 1)] = l;
    out[(i << 1) + 1] = r;
  }

  buffer->mAudioDataByteSize = frames * sizeof(int16_t) * 2;
  if (AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr) != noErr) {
    ctx->finished.store(true, std::memory_order_release);
  }
}

bool RunRealtime(IEngineRunner* runner,
                 const ControlState& initial,
                 const std::vector<AutomationEvent>& events,
                 EngineKind engine,
                 double duration_seconds) {
  const uint64_t total_samples =
      (duration_seconds > 0.0) ? static_cast<uint64_t>(std::llround(duration_seconds * static_cast<double>(kSampleRate))) : 0U;

  AudioStreamBasicDescription format{};
  format.mSampleRate = kSampleRate;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  format.mBitsPerChannel = 16;
  format.mChannelsPerFrame = 2;
  format.mFramesPerPacket = 1;
  format.mBytesPerFrame = 4;
  format.mBytesPerPacket = 4;

  RealtimeContext ctx(runner, initial, events, total_samples);
  AudioQueueRef queue = nullptr;

  OSStatus status = AudioQueueNewOutput(&format, AudioQueueCallback, &ctx, nullptr, nullptr, 0, &queue);
  if (status != noErr) {
    std::cerr << "Failed to create AudioQueue output (OSStatus " << int(status) << ")\n";
    return false;
  }

  constexpr int kNumBuffers = 3;
  constexpr uint32_t kFramesPerBuffer = 256;
  constexpr uint32_t kBytesPerBuffer = kFramesPerBuffer * sizeof(int16_t) * 2;

  AudioQueueBufferRef buffers[kNumBuffers] = {};
  for (int i = 0; i < kNumBuffers; ++i) {
    status = AudioQueueAllocateBuffer(queue, kBytesPerBuffer, &buffers[i]);
    if (status != noErr) {
      std::cerr << "Failed to allocate AudioQueue buffer (OSStatus " << int(status) << ")\n";
      AudioQueueDispose(queue, true);
      return false;
    }
    AudioQueueCallback(&ctx, queue, buffers[i]);
  }

  g_stop_requested.store(false, std::memory_order_relaxed);
  using SigHandler = void (*)(int);
  SigHandler old_handler = std::signal(SIGINT, OnSigInt);

  status = AudioQueueStart(queue, nullptr);
  if (status != noErr) {
    std::cerr << "Failed to start AudioQueue (OSStatus " << int(status) << ")\n";
    AudioQueueDispose(queue, true);
    std::signal(SIGINT, old_handler);
    return false;
  }

  std::cout << "Realtime started"
            << " engine=" << EngineName(engine)
            << " sample_rate=" << kSampleRate
            << " control_div=" << kControlDivisor;
  if (duration_seconds > 0.0) {
    std::cout << " duration=" << duration_seconds << "s";
  } else {
    std::cout << " (press Ctrl+C to stop)";
  }
  std::cout << "\n";

  while (!ctx.finished.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  AudioQueueStop(queue, true);
  AudioQueueDispose(queue, true);
  std::signal(SIGINT, old_handler);

  std::cout << "Realtime stopped"
            << " rendered_samples=" << ctx.stepper.SampleIndex()
            << " rendered_seconds=" << (double(ctx.stepper.SampleIndex()) / double(kSampleRate)) << "\n";
  return true;
}
#else
bool RunRealtime(IEngineRunner*,
                 const ControlState&,
                 const std::vector<AutomationEvent>&,
                 EngineKind,
                 double) {
  std::cerr << "--realtime is only supported on macOS in this build.\n";
  return false;
}
#endif

}  // namespace

int main(int argc, char** argv) {
  CliOptions opts;
  if (!ParseCli(argc, argv, opts)) {
    if (opts.help_requested || argc == 1) {
      PrintUsage(argv[0]);
      return 0;
    }
    return 1;
  }

  auto runner = CreateRunner(opts.engine);
  if (!runner) {
    std::cerr << "Failed to create engine runner\n";
    return 1;
  }

  ControlState state;
  state.knob_main = opts.knob_main;
  state.knob_x = opts.knob_x;
  state.knob_y = opts.knob_y;
  state.audio1 = opts.audio1;
  state.audio2 = opts.audio2;
  state.audio1_connected = opts.audio1_connected;
  state.audio2_connected = opts.audio2_connected;
  state.alt = opts.alt;

  std::vector<AutomationEvent> events;
  if (!opts.script_path.empty()) {
    if (!LoadAutomationCsv(opts.script_path, state, events)) {
      return 1;
    }
  }

  if (opts.realtime) {
    double duration = -1.0;
    if (opts.duration_set) {
      duration = opts.duration;
    } else if (opts.seconds_set) {
      duration = opts.seconds;
    }

    return RunRealtime(runner.get(), state, events, opts.engine, duration) ? 0 : 1;
  }

  return RenderOffline(runner.get(), state, events, opts.out_path, opts.seconds, opts.engine) ? 0 : 1;
}
