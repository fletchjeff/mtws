# Agent Reference for `mtws`

Use this as a reference catalog when implementing or refactoring code in `mtws`.
The goal is to increase reuse of proven helpers from `reference/utility_pair/` and
to steer oscillator work toward the existing `mtws` engine patterns.

## Oscillator Skill Map
- Use `.ai/skills/mtws-oscillator-design/` when shaping sonic intent, control maps, helper reuse plans, and hot-path risk before code edits.
- Use `.ai/skills/mtws-oscillator-solo/` when creating a standalone oscillator target under `knots/solo_engines/<engine>/` with `knots/solo_engines/solo_common/`.
- Use `.ai/skills/mtws-oscillator-integration/` when wiring an oscillator into `knots/src/engines/`, `knots/src/core/shared_state.h`, the engine registry, and the user docs.

## Most Useful Repo References
- `docs/engines/`: current per-engine control maps, sonic goals, and hardware test checklists
- `docs/MTWS_USER_GUIDE.md`: current integrated build behavior that overrides standalone docs when they differ
- `knots/solo_engines/solo_common/`: standalone control/router scaffold
- `knots/src/engines/`: integrated slot interface and current engine patterns
- `knots/src/engines/bender_engine.cpp`: simple `ControlTick()` to `RenderSample()` split with Utility-Pair-style optimizations
- `knots/src/engines/din_sum_engine.cpp`: heavier control-rate caching and hot-path optimization examples

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
- `ComputerCard.h`: platform I/O helpers and edge-detect behavior
- `delayline.h`: reusable delay line with interpolation support
- `bernoulligate.h`: probability gate helper
- `t185_state.h`: clock state and trigger-pattern helpers

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
- Keep `ProcessSample()` under the time budget of about `20us` at `48kHz`
- Prefer fixed-point and integer math
- Precompute tables where possible
- Clamp indices and CV sums before lookup or table access
- Use `__not_in_flash_func()` for hot-path code where appropriate
- Move heavy work such as USB MIDI tasks off the audio core
- `mtws` baseline runs RP2040 at `200MHz`; this adds compute headroom but does not change 48kHz audio timing
- Consider clocks above `200MHz` only as an explicit experiment with dedicated stability testing

## Code Review Checklist for Plans
Before proposing code changes, explicitly answer:

1. Which helper(s) from `reference/utility_pair/` did I check?
2. Which helper(s) will I reuse or adapt?
3. If not reusing, why not?
4. What is the performance impact in `ProcessSample()`?
5. What build, flash, and hardware test will confirm behavior?
