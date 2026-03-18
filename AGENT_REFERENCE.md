# Agent Reference for `mtws`

Use this as a reference catalog when implementing or refactoring code in `mtws`.
The goal is to increase reuse of proven helpers from `reference/utility_pair/`,
reuse proven platform behavior from `ComputerCard.h` and the workshop examples,
and steer oscillator work toward the existing `mtws` and `10_twists` patterns.

## Oscillator Skill Map
- Use `.ai/skills/mtws-oscillator-design/` when shaping sonic intent, control maps, helper reuse plans, and hot-path risk before code edits.
- Use `.ai/skills/mtws-oscillator-solo/` when creating a standalone oscillator target under `knots/solo_engines/<engine>/` with `knots/solo_engines/solo_common/`.
- Use `.ai/skills/mtws-oscillator-integration/` when wiring an oscillator into `knots/src/engines/`, `knots/src/core/shared_state.h`, the engine registry, and the user docs.

## Most Useful Repo References
- `docs/engines/`: current per-engine control maps, sonic goals, and hardware test checklists
- `docs/MTWS_USER_GUIDE.md`: current integrated build behavior that overrides standalone docs when they differ
- `knots/solo_engines/solo_common/`: standalone control/router scaffold
- `knots/src/main.cpp`: current integrated host architecture, core split, control cadence, and published-frame flow
- `knots/src/engines/`: integrated slot interface and current engine patterns
- `knots/src/engines/bender_engine.cpp`: simple `ControlTick()` to `RenderSample()` split with Utility-Pair-style optimizations
- `knots/src/engines/din_sum_engine.cpp`: heavier control-rate caching and hot-path optimization examples

## Current `mtws` Host Architecture
- `ComputerCard::AudioWorker()` is the underlying realtime audio loop that drives `ProcessSample()` at `48kHz`.
- In `knots/src/main.cpp`, `MTWSApp::ProcessSample()` launches `Core1Entry` on core 1 the first time audio runs.
- `Core1Entry` calls `MTWSApp::ControlAndMIDIWorkerCore()`, which runs the non-audio loop.
- That non-audio loop polls `MIDIWorker::Poll()`, and `MIDIWorker::Poll()` calls `tud_task()`, so USB MIDI work stays off the audio core path.
- The integrated host publishes control frames for the audio path instead of recomputing control work every sample.
- `ControlTick()` in the integrated host runs every 48 audio samples via `kControlDivisor = 48`, about `1kHz` at `48kHz`.
- Engine code should usually treat `ControlTick()` as the control-domain preprocessing step and `RenderSample()` as the audio-domain hot path.

## Reference Routing by Problem
- Reusable DSP helper, math primitive, random source, filter, delay, or small oscillator block:
  start with `reference/utility_pair/`.
- Workshop System platform API, jack behavior, calibrated I/O helpers, edge detection, or module conventions:
  start with repo-root `ComputerCard.h`.
- USB, MIDI, second-core work, calibration, sample upload, or web tooling:
  start with `reference/workshop_computer_examples/`.
- Larger oscillator architecture, parameter interpolation, resources/settings structure, or USB worker patterns:
  start with `reference/10_twists/`.
- Existing `mtws` engine implementation choice or hot-path structure:
  start with `knots/src/engines/` and `knots/solo_engines/`.

## Reuse Priority
Before adding a new helper, check whether an existing Utility-Pair implementation already covers it:
1. Random and probability
2. Curve conversion such as expo and V-oct
3. Filtering, smoothing, and DC blocking
4. Delay and buffer utilities
5. Oscillators and reusable DSP blocks

## Utility-Pair Source Map
These are the first files to inspect in `reference/utility_pair/`:
- `main.cpp`: `rnd12`, `rnd24`, `rndi32`, `cabs`, `clip`, `Exp4000`, `ExpVoct`, `ExpNote`, and large pattern examples
- `delayline.h`: reusable delay line with interpolation support
- `Saw.h`: anti-aliased saw oscillator helper
- `divider.h`: clock divider helper
- `bernoulligate.h`: probability gate helper
- `t185_state.h`: clock state and trigger-pattern helpers

## Workshop System Platform Reference
- `ComputerCard.h`: repo-root Workshop System platform API.
- Use it first when the task depends on:
  - `KnobVal(...)`, `AudioIn*()`, `CVIn*()`, `AudioOut*()`, `CVOut*()`, or `Pulse*()`
  - `Connected(...)` behavior and jack repurposing logic
  - calibrated helpers such as MIDI-to-CV output functions
  - switch and edge-detect behavior

## Workshop Computer Example Map
Inspect `reference/workshop_computer_examples/` when the task is more about board behavior than core DSP:
- `second_core/main.cpp`: split work across both RP2040 cores
- `midi_device/` and `midi_host/`: USB MIDI device and host patterns
- `midi_device_host/`: combined MIDI device/host reference
- `usb_detect/main.cpp` and `usb_serial/main.cpp`: USB connection and serial patterns
- `calibrated_cv_out/main.cpp`: calibrated CV output usage
- `sample_upload/`: sample transfer and browser-assisted loading flow
- `web_interface/`: simple web UI plus TinyUSB descriptor example

## 10 Twists Source Map
Inspect `reference/10_twists/` when you need a larger integrated oscillator reference:
- `macro_oscillator.h` and `macro_oscillator.cc`: higher-level oscillator organization
- `digital_oscillator.h` and `digital_oscillator.cc`: digital oscillator structure
- `analog_oscillator.h` and `analog_oscillator.cc`: analog-style oscillator structure
- `parameter_interpolation.h`: control smoothing and interpolation patterns
- `resources.h` and `resources.cc`: wavetable/resource layout
- `settings.h` and `settings.cc`: persistent settings and configuration patterns
- `usb_worker.h` and `usb_worker.cc`: USB work moved outside the audio hot path
- `ui.h` and `ui.cc`: UI/service split patterns

## Random Number Generation
Fast deterministic random generators suitable for embedded and audio-rate usage.

- `uint32_t rnd12()`
  - Returns 12-bit random value (`0..4095`)
  - Good for probabilities, noise amplitudes, and knob-like randomness

- `uint32_t rnd24()`
  - Returns 24-bit random value (`0..16777215`)
  - Better resolution for modulation, interpolation, and dithering-like tasks

- `int32_t rndi32()`
  - Returns signed 32-bit random
  - Useful for full-range noise and internal random state logic

Typical uses:
- probability gates
- clock variation and ratcheting decisions
- noise sources
- sample-and-hold values

## Math Utilities
Small low-cost helpers for realtime-safe arithmetic.

- `int32_t cabs(int32_t a)`
  - Integer absolute value

- `void clip(int32_t &a)`
  - Clips to about `+-2047`
  - Use before `AudioOut*()` or `CVOut*()` when signal bounds are uncertain

Guideline:
Always clamp sums and feedback paths before output if there is any chance of overflow or out-of-range values.

## Exponential and Musical Conversion Helpers
These are high-value helpers and should be reused whenever pitch or musical taper is involved.

- `int32_t Exp4000(int32_t in)`
  - Input: `0..2047`
  - Output: exponential mapping with about `4000:1` ratio
  - Use for time constants, rate controls, envelope timing, and nonlinear tapers

- `int32_t ExpVoct(int32_t in)`
  - Input: `0..4095`
  - Output: V/oct-style frequency scaling
  - Use for oscillator pitch, tuned delay, and tracking controls
  - About `455` values per octave, max around audio-rate frequency range

- `int32_t ExpNote(int32_t in)`
  - Input: MIDI note `0..127`
  - Output: frequency scaling or phase increment form
  - Use for MIDI-controlled synth behavior

Recommended pattern for pitch sum and clamp:
```cpp
int32_t pitch = KnobVal(Knob::Main) + CVIn1();
if (pitch < 0) pitch = 0;
if (pitch > 4095) pitch = 4095;
int32_t freq = ExpVoct(pitch);
```

## Filter Helpers
Low-CPU one-pole filters that are ideal for the RP2040 audio loop.

### High-pass
- `int32_t highpass_process(int32_t *state, int32_t coeff, int32_t input)`
  - `state`: persistent filter state, init `0`
  - `coeff`: cutoff control, `200` is about `8Hz` at `48kHz`
  - Returns high-passed signal

Use for:
- delay or reverb feedback loops
- removing offsets from nonlinear processing
- stabilizing recirculating paths

### Low-pass
- `int32_t lowpass_process(int32_t *state, int32_t coeff, int32_t input)`
  - `state`: persistent filter state, init `0`
  - `coeff`: `0..65536`, higher means higher cutoff
  - Returns low-passed signal

Use for:
- parameter smoothing
- envelope slewing
- tone softening and simple filtering

## Delay and Buffer Utilities

### `DelayLine<bufSize, storage>`
Template circular delay buffer with interpolation support.

Core methods:
- `Write(sample)`
- `ReadRaw(delaySamples)`
- `ReadInterp(delay128ths)`

Why reuse it:
- solves buffer wraparound correctly
- interpolation is already implemented
- suitable base for delay, chorus, flanger, comb, and Karplus-Strong work

Typical pattern:
```cpp
DelayLine<48000> delay;

delay.Write(AudioIn1());
int32_t d = delay.ReadInterp((24000 << 7));
AudioOut1(d);
```

## Oscillator and Generator Helpers

### `Saw`
Anti-aliased saw oscillator from Utility-Pair, better than a naive ramp at high pitch.

Key methods:
- `SetFreq(freq)`
- `Tick()`

Pattern:
```cpp
Saw osc;
int32_t freq = ExpVoct(KnobVal(Knob::Main));
osc.SetFreq(freq);
int32_t s = osc.Tick();
AudioOut1(s >> 20);
```

## Clock and Probability Helpers

### `Divider`
Clock divider class for pulse streams.

Methods:
- `Set(divisor)`
- `SetResetPhase(divisor)`
- `Step(risingEdge)`

Use for:
- clock divisions
- trigger thinning
- rhythmic structures

### `bernoulli_gate`
Probability gate or toggle generator.

Pattern:
```cpp
bernoulli_gate gate;
gate.set_toggle(false);
bool out = gate.step(KnobVal(Knob::Main), PulseIn1RisingEdge());
PulseOut1(out);
```

Use for:
- chance-based triggers and gates
- random switching behavior
- generative rhythm tools

## Template DSP Classes in Utility-Pair
Treat these as implementation templates or pattern libraries to reuse or strip down:

- `looper<I>`
- `wavefolder<I>`
- `delay<I>`
- `flanger<I>`
- `slowlfo<I>`
- `karplusstrong<I>`

Guidance:
- reuse architecture and state update patterns
- copy only required pieces
- keep the hot path minimal after porting

## Preferred `mtws` Bootstrap Patterns
- Start new standalone oscillator scaffolds from the simpler `knots/solo_engines/bender/` structure.
- Use `knots/solo_engines/sawsome/` as a reference when you need multi-voice spread, stereo gain maps, or PolyBLEP-like per-voice control frames.
- Use `knots/src/engines/bender_engine.cpp` as the first integrated-slot template because it shows a clear `ControlTick()` to `RenderSample()` split and Utility-Pair-inspired optimizations.
- Use `knots/src/engines/din_sum_engine.cpp` when the hot path needs heavier caching or control-rate precomputation.

## Realtime Performance Rules
Apply the repo-wide performance policy from `AGENTS.md`, then use these reference-specific reminders:
- Precompute tables where possible
- Clamp indices and CV sums before lookup or table access
- Cache control-domain values before the audio loop where possible
- Prefer control-rate preprocessing over repeated per-sample recomputation
- Preserve the current host split where MIDI/USB/control work happens off the audio core path unless there is a measured reason to change it
- Preserve the current host control cadence of 48 audio samples unless there is a measured reason to change it

## Code Review Checklist for Plans
Before proposing code changes, explicitly answer:

1. Which helper(s) from `reference/utility_pair/` did I check?
2. Which helper(s) will I reuse or adapt?
3. If not reusing, why not?
4. What is the performance impact in `ProcessSample()`?
5. What build, flash, and hardware test will confirm behavior?
