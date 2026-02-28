# CODEX.md

This file combines the repository workflow rules from `AGENTS.md` with the hardware/library reference from `CLAUDE.md` for use by Codex when helping with the Music Thing Modular Workshop System.

## IMPORTANT
Any git operations should only be done on the `mtws_additive`, all other folders and files in the root direct must be ignored. 

You will always write small amounts of code incrementally test performance through the build / flash testing loop. 

Always check with the user first before adding code by providing a detailed output of what you intend to change first.


## Repository Guidelines (Workspace + Workflow)

## Project Structure & Module Organization
This workspace contains several related repositories for the Music Thing Workshop System:
- `mtws_additive/`: C++17 Pico SDK additive/prototype firmware. Main code lives in `prex/*` and `examples/*` with per-target `main.cpp`.
- `Utility-Pair/`: Utility Pair card firmware. Core DSP/utilities are in `src/`; audio loop assets are in `loops/`.
- `Workshop_Computer/`: upstream demos, release cards, and docs; static site generator is in `tools/sitegen/`.
- `pico-sdk/` and `reference/`: vendored SDK/reference code. Avoid editing unless intentionally syncing upstream.

Keep generated artifacts in `build/` directories and do not commit them.

## Build, Test, and Development Commands
- `cd mtws_additive && mkdir -p build && cd build && cmake .. && make -j`  
  Build additive/prototype UF2 targets.
- `cd Utility-Pair && mkdir -p build && cd build && cmake .. && make -j`  
  Build Utility Pair targets. Limit scope by trimming `targetlist` in `Utility-Pair/CMakeLists.txt` (full 625-target build is very large/slow).
- `cd Workshop_Computer && npm ci --prefix tools/sitegen && npm run build --prefix tools/sitegen`  
  Rebuild the website into `Workshop_Computer/site/`.
- `cd Workshop_Computer && python .github/scripts/update-readme.py`  
  Regenerate `releases/README.md` from `releases/*/info.yaml`.

## Coding Style & Naming Conventions
- Use C++17 for firmware projects (as configured in project `CMakeLists.txt` files).
- Follow existing local style in each file: brace and indentation patterns vary between subprojects; match surrounding code.
- Prefer descriptive lowercase filenames and helper names (for example `bernoulligate.h`, `rnd24`).
- Keep `ProcessSample()` paths deterministic and lightweight; prefer integer/fixed-point math for realtime audio code.

## Testing Guidelines
There is no automated unit-test suite in `mtws_additive` or `Utility-Pair`.
- For every change, perform a clean build of the affected target(s).
- Flash the generated `.uf2` and run a hardware smoke test for impacted paths (audio I/O, CV, pulse, switch/LED behavior).
- Document manual test steps and observed results in the PR.

## Commit & Pull Request Guidelines
- Follow existing commit style: short, imperative, and specific (for example: `Fix mode parity for X/morph mapping`, `Add MIT License to the project`).
- Keep commits scoped to one logical change and one subproject.
- PRs should include purpose, touched paths, build commands executed, and hardware validation notes. Add screenshots/wave captures when behavior or UI/output changes.

---

## Workshop Computer Reference (Hardware + ComputerCard + DSP Helpers)

## Project Overview

The Workshop Computer is a Raspberry Pi RP2040-based music module for the Music Thing Modular Workshop System. This repository contains:
- Demo/starter code for various development platforms
- Released "program cards" (numbered projects 00-99) that implement different audio/CV functions
- ComputerCard library for simplified hardware interaction

Hardware capabilities:
- 2x audio inputs/outputs (12-bit, 48kHz via SPI DAC)
- 2x CV inputs/outputs (12-bit, -6V to +6V)
- 2x pulse inputs/outputs (5V digital)
- 3 knobs (Main, X, Y) and 1 three-position switch
- 6 LEDs (brightness controllable via PWM)
- USB MIDI device/host support
- Jack detection via normalisation probe

## Development Platforms

The codebase supports multiple development approaches:

### Pico SDK + ComputerCard (Recommended)
- Location: `Demonstrations+HelloWorlds/PicoSDK/ComputerCard/`
- Most mature and feature-complete library
- Header-only C++ library providing 48kHz audio callback framework
- Build system: CMake
- Build commands (from `ComputerCard/build/`):
  ```bash
  cmake ..
  make
  ```
- Output: `.uf2` files in `build/` directory

### Arduino IDE
- Uses earlephilhower Arduino-Pico core
- Can use ComputerCard library by including `ComputerCard.h` in sketch directory
- Important: Set Tools -> USB Stack to "No USB" to avoid normalisation probe interference
- Build: Sketch -> Upload or Sketch -> Export Compiled Binary

### CircuitPython
- Location: `Demonstrations+HelloWorlds/CircuitPython/`
- Uses `mtm_computer.py` helper library
- Note: Audio output via SPI DAC not currently supported
- CV output and LED control work

### MicroPython
- Location: `Demonstrations+HelloWorlds/Micropython/`
- Also see external libraries: pyworkshopsystem, rwmodular examples
- Flash official MicroPython UF2 to board first

## Program Card Structure

Releases are organized in `releases/NN_name/` folders where NN is the card number (00-99).

Each release folder contains:
- `info.yaml` - Metadata (Description, Version, Language, Creator, Status)
- Source code and/or precompiled `.uf2` files
- `README.md` - Card-specific documentation
- `build/` directory (for compiled projects)

The `releases/README.md` is auto-generated by `.github/scripts/update-readme.py` which reads all `info.yaml` files and creates a table. Do not manually edit `releases/README.md`.

### Adding a New Program Card

1. Create folder: `releases/NN_cardname/`
2. Add `info.yaml` with fields: Description, Version, Language, Creator, Status
3. Add source code and/or compiled `.uf2` files
4. Add README.md with usage instructions
5. The GitHub Action will auto-update `releases/README.md` on push to main

## ComputerCard Library Architecture

The ComputerCard class provides a callback-based framework:

### Core Pattern
```cpp
#include "ComputerCard.h"

class MyCard : public ComputerCard {
public:
    virtual void ProcessSample() {  // Called at 48kHz
        // Your audio/CV processing code here
    }
};

int main() {
    MyCard card;
    card.EnableNormalisationProbe();  // Optional: for jack detection
    card.Run();  // Blocking call
}
```

### Performance Constraints
- `ProcessSample()` must complete in ~20μs (1/48000 sec)
- Use integer arithmetic, not floating point (RP2040 has no FPU)
- Fixed-point representation: signed 32-bit integers with bit-shifting
- For lengthy operations: use second core, lookup tables, or split across samples
- Audio-rate code should use `__not_in_flash_func()` to run from RAM

### Hardware Access Methods (inside ProcessSample)
- Audio: `AudioIn1()`, `AudioIn2()`, `AudioOut1(val)`, `AudioOut2(val)` (±2048 range)
- CV: `CVIn1()`, `CVIn2()`, `CVOut1(val)`, `CVOut2(val)` (±2048 range)
- CV Precise: `CVOut1Precise(val)` (±262144 range, 19-bit sigma-delta)
- Pulses: `PulseIn1()`, `PulseIn1RisingEdge()`, `PulseOut1(bool)`
- Knobs: `KnobVal(Knob::Main)`, `KnobVal(Knob::X)`, `KnobVal(Knob::Y)` (0-4095)
- Switch: `SwitchVal()` returns `Up`, `Middle`, or `Down`; `SwitchChanged()` detects changes
- LEDs: `LedOn(index)`, `LedOff(index)`, `LedBrightness(index, 0-4095)`
- Jack detection: `Connected(Input::Audio1)`, `Disconnected(Input::CV2)`, etc.

### Examples
Available in `ComputerCard/examples/`:
- `passthrough` - Basic I/O demonstration
- `sample_and_hold` - Typical card implementation
- `midi_device`, `midi_host`, `midi_device_host` - USB MIDI examples
- `second_core` - Multi-core processing
- `normalisation_probe` - Jack detection
- `sample_upload` - WAV file upload/playback interface

## Utility Functions and Helper Classes

The `Utility-Pair/src/` directory contains battle-tested utility functions and classes for common DSP operations. These are production-quality implementations optimized for the RP2040.

### Random Number Generation

Fast, deterministic random number generators (Linear Congruential Generator):

```cpp
uint32_t rnd12()   // Returns 12-bit random (0-4095)
uint32_t rnd24()   // Returns 24-bit random (0-16777215)
int32_t rndi32()   // Returns full 32-bit signed random
```

**Usage:** Clock dividers, probability gates, noise generation, sample & hold

### Math Utilities

```cpp
int32_t cabs(int32_t a)         // Absolute value (integer)
void clip(int32_t &a)           // Clip to ±2047 (12-bit DAC range)
```

### Exponential Conversion Functions

Essential for musical frequency control - converts linear knob/CV values to exponential frequency scaling:

```cpp
int32_t Exp4000(int32_t in)      // Input: 0-2047, Output: exponential over ~4000:1 ratio
                                  // Use for: general exponential curves, time controls

int32_t ExpVoct(int32_t in)      // Input: 0-4095 (CV/knob), Output: V/oct scaling
                                  // Use for: pitch control, VCO frequency
                                  // ~455 values per octave, max ~20kHz

int32_t ExpNote(int32_t in)      // Input: 0-127 (MIDI note), Output: frequency
                                  // Use for: MIDI note to frequency conversion
```

**Example - Pitch control:**
```cpp
int32_t knob = KnobVal(Knob::Main);
int32_t cv = CVIn1();
int32_t combined = knob + cv;
if (combined < 0) combined = 0;
if (combined > 4095) combined = 4095;
int32_t frequency = ExpVoct(combined);  // Now use as phase increment
```

### Filter Functions

Simple but effective one-pole filters (fast, low CPU usage):

```cpp
// High-pass filter (removes DC offset, useful for feedback loops)
int32_t highpass_process(int32_t *state, int32_t coeff, int32_t input)
// state: filter state variable (initialize to 0)
// coeff: cutoff frequency (200 = ~8Hz at 48kHz, higher = higher cutoff)
// Returns: high-passed output

// Low-pass filter (smoothing, tone control)
int32_t lowpass_process(int32_t *state, int32_t coeff, int32_t input)
// state: filter state variable (initialize to 0)
// coeff: cutoff frequency (0-65536, higher = higher cutoff)
// Returns: low-passed output
```

**Example - DC blocking in delay line:**
```cpp
int32_t hpf_state = 0;  // In class member variables
int32_t feedback_signal = highpass_process(&hpf_state, 200, delayed_output);
```

### Helper Classes

#### DelayLine<bufSize, storage>
Template class for circular buffer delays with interpolation:

```cpp
DelayLine<48000> delay;  // 1 second at 48kHz (default int16_t storage)

void ProcessSample() {
    int32_t input = AudioIn1();

    // Write to delay line
    delay.Write(input);

    // Read with interpolation (delay in 128ths of a sample for smooth modulation)
    int32_t delayed = delay.ReadInterp(24000 << 7);  // 0.5 second delay

    // Or raw read (delay in samples, no interpolation)
    int32_t delayed = delay.ReadRaw(24000);  // 0.5 second delay

    AudioOut1(delayed);
}
```

**Use cases:** Echo/delay effects, chorus, flangers, reverb, comb filters, Karplus-Strong

#### Saw
Anti-aliased sawtooth oscillator (uses Välimäki algorithm):

```cpp
Saw osc;

void ProcessSample() {
    int32_t freq = ExpVoct(KnobVal(Knob::Main));
    osc.SetFreq(freq);  // freq is ~89478 per Hz at 48kHz

    int32_t sample = osc.Tick();  // Returns ±2^30 range
    AudioOut1(sample >> 20);      // Scale down to ±2048
}
```

**Benefits:** Much cleaner than naive sawtooth, reduced aliasing artifacts

#### Divider
Clock divider with configurable division ratio:

```cpp
Divider div;

void ProcessSample() {
    div.Set(8);  // Divide by 8

    bool rising = PulseIn1RisingEdge();
    bool output = div.Step(rising);

    PulseOut1(output);  // Outputs pulse every 8th input pulse
}
```

**Methods:**
- `Set(divisor)` - Change division ratio
- `SetResetPhase(divisor)` - Change ratio and reset counter
- `Step(risingEdge)` - Returns true when division triggers

#### bernoulli_gate
Probability-based gate generator (Mutable Instruments Branches-style):

```cpp
bernoulli_gate gate;

void ProcessSample() {
    gate.set_toggle(false);  // false = gate mode, true = toggle mode

    int32_t probability = KnobVal(Knob::Main);  // 0-4095
    bool trigger = PulseIn1RisingEdge();

    bool output = gate.step(probability, trigger);
    PulseOut1(output);
}
```

**Modes:**
- Gate mode: Probability of gate going high on each trigger
- Toggle mode: Probability of toggling state on each trigger

### Template Classes (Advanced)

The `main.cpp` also includes full template implementations of:

- **looper<I>** - Beat-synced audio looper with clock division
- **wavefolder<I>** - Anti-aliased wavefolder with BLEP
- **delay<I>** - Full-featured delay with feedback control
- **flanger<I>** - Stereo flanger with LFO
- **slowlfo<I>** - Precision slow LFO with sine lookup
- **karplusstrong<I>** - Physical modeling string synthesis

These can be used as templates for building similar effects or extended for new functionality.

### Best Practices

1. **Always use `__not_in_flash_func()` for hot path code** - Runs from RAM (faster)
2. **Pre-calculate lookup tables** - See `exp4000_vals[]`, `voct_vals[]`, sine tables
3. **Use template parameters for channel selection** - Enables stereo without code duplication
4. **Fixed-point throughout** - No floating-point in `ProcessSample()`
5. **Clamp inputs** - Always validate CV/knob sums before using as array indices

## USB MIDI
- TinyUSB `tud_task()` takes >20μs, must run on different core from audio
- See `midi_device` example for multicore pattern
- Computer can act as USB host (connect MIDI devices) or device (connect to computer)
- Computer v1.1+ hardware: `USBPowerState()` detects DFP/UFP mode

## Flashing Firmware

1. Enter bootloader mode: Hold BOOTSEL (under main knob) + press RESET
2. Computer appears as `RPI-RP2` drive
3. Drag `.uf2` file to drive
4. Computer auto-reboots with new firmware

For batch flashing (macOS), see `documentation/uploads.md` for shell loop scripts.

## Clock Speed
- Default: 125MHz
- Pico SDK 2.1.1+: up to 200MHz supported via `set_sys_clock_khz(200000, true);`
- Higher speeds give more headroom for ProcessSample()

## Documentation Resources
- Hardware details: [Google Doc](https://docs.google.com/document/d/1NsRewxAu9X8dQMUTdN0eeJeRCr0HmU0pUjpKB4gM-xo/edit?usp=sharing)
- Program card browser: https://tomwhitwell.github.io/Workshop_Computer/
- Discord: Link in documentation
- Homepage: https://www.musicthing.co.uk/workshopsystem/

## ComputerCard Version History
Check `ComputerCard.h` for version number (0.2.6+) or MD5 checksum (earlier versions).
Major changes: v0.2.0 fixed audio I/O sign/order, v0.2.6 added 19-bit CV output, v0.2.7 improved ADC linearity.
