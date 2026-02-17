# Wavetable Synthesizer Documentation

## Overview

This is a **wavetable synthesizer** that morphs between 60 pre-recorded waveforms to create evolving harmonic timbres. Think of it like having 60 different "instrument sounds" that you can smoothly blend between using a knob.

**Key Features:**
- 60 waveforms with smooth interpolation between them
- Pitch control covering ±2 octaves (110Hz to 1760Hz)
- Real-time audio processing at 48kHz sample rate
- Runs on RP2040 microcontroller (264KB RAM, 125MHz CPU, no floating-point unit)

**Controls:**
- **Main Knob**: Pitch/frequency control (center = 440Hz / A4)
- **X Knob**: Wavetable position (morphs through all 60 waveforms)

---

## File Structure

### 1. `wavetable_data.h` - The Audio Data

```cpp
const int16_t wavetable[60][600] = { ... };
```

**Python Equivalent:**
```python
import numpy as np
wavetable = np.array([[...], [...], ...])  # Shape: (60, 600)
```

**What it contains:**
- A 2D array of 16-bit signed integers
- **60 waveforms** (rows) × **600 samples per waveform** (columns)
- Each sample ranges from -32768 to +32767 (16-bit audio)
- Total size: 36,000 samples × 2 bytes = **72KB**

**How it was created:**
1. Started with `AKWF_60_ordered.wav` (44.1kHz, 16-bit mono, 36,000 samples)
2. Extracted samples using Python's `wave` module
3. Split into 60 chunks of 600 samples each
4. Formatted as C array for embedding in firmware

**Think of it as:** A NumPy array where each row is one "instrument sound" sampled at different points in its cycle.

---

### 2. `main.cpp` - The Synthesizer Logic

This implements the real-time audio processing algorithm.

---

## Core Concepts (Python Background Translation)

### C++ vs Python Differences

| Concept | Python | C++ (This Code) |
|---------|--------|-----------------|
| **Variable types** | Dynamic typing (`x = 5`) | Static typing (`int32_t x = 5;`) |
| **Arrays** | Lists/NumPy arrays | Fixed-size arrays `int16_t arr[600]` |
| **Math** | Floating-point by default | **Integer-only** (no FPU on RP2040) |
| **Classes** | `class Foo:` | `class Foo { };` (note semicolon) |
| **Methods** | `def method(self):` | `void method() { }` |
| **Inheritance** | `class Child(Parent):` | `class Child : public Parent { }` |

### Fixed-Point Arithmetic (Critical!)

Since the RP2040 has no floating-point unit (FPU), all calculations use **fixed-point arithmetic** - integers that represent fractional values.

**Python analogy:**
```python
# Floating-point (what you'd normally do in Python):
x = 0.5
y = 0.25
result = x * y  # = 0.125

# Fixed-point (what this code does):
# Represent decimals as integers with implicit decimal point
# Q16 format: multiply by 2^16 (65536) to store decimals
x_fixed = int(0.5 * 65536)    # = 32768
y_fixed = int(0.25 * 65536)   # = 16384
result_fixed = (x_fixed * y_fixed) >> 16  # Shift right to rescale
result = result_fixed / 65536  # = 0.125
```

**Why use fixed-point?**
- Integer operations are **100x faster** on RP2040 than floating-point
- We have only ~20 microseconds per audio sample (48kHz rate)
- Fixed-point is deterministic and predictable

**Common notation in this code:**
- `Q16` or `_fp16` = 16-bit fractional precision (1.0 = 65536)
- `>> 16` = Divide by 65536 (shift right 16 bits)
- `<< 16` = Multiply by 65536 (shift left 16 bits)

---

## Class Structure

```cpp
class WavetableSynth60 : public ComputerCard
```

**Python equivalent:**
```python
class WavetableSynth60(ComputerCard):
    def __init__(self):
        super().__init__()
```

**Inheritance:** `ComputerCard` is the base class that provides hardware access methods:
- `KnobVal(Knob::Main)` - Read knob position (0-4095)
- `AudioOut1(value)` - Output audio sample (-2048 to +2047)
- `ProcessSample()` - Called 48,000 times per second

---

## Member Variables

```cpp
constexpr static unsigned numWaveforms = 60;
constexpr static unsigned samplesPerWaveform = 600;
```

**Python equivalent:**
```python
NUM_WAVEFORMS = 60  # Class constant
SAMPLES_PER_WAVEFORM = 600
```

**`constexpr static`**: Compile-time constants (like `Final` in Python 3.8+). These never change and get optimized away.

---

```cpp
uint32_t phase;
```

**What it is:** A 32-bit unsigned integer representing the current position in the waveform cycle.

**Range:** 0 to 4,294,967,295 (2³² - 1)
- `phase = 0` → Start of waveform (0°)
- `phase = 2^31` → Middle of waveform (180°)
- `phase = 2^32 - 1` → End of waveform (360°)

**Why 32 bits?** Gives extremely fine frequency resolution without floats. When phase overflows (wraps from 2³² back to 0), it automatically creates a repeating waveform.

**Python analogy:**
```python
phase = 0  # Current position in waveform
phase_increment = 39370534  # How much to advance each sample

for sample in range(48000):  # 1 second of audio
    # Read from wavetable at current phase
    position = (phase * 600) // (2**32)  # Map phase to 0-599
    audio_out = wavetable[0][position]

    # Advance phase (wraps automatically with modulo)
    phase = (phase + phase_increment) % (2**32)
```

---

```cpp
constexpr static uint32_t basePhaseInc = 39370534;
```

**What it is:** The amount to add to `phase` each sample to produce 440Hz (A4 note).

**Calculation:**
```
frequency = 440 Hz
sample_rate = 48000 Hz
phase_increment = (2^32 * frequency) / sample_rate
                = (4294967296 * 440) / 48000
                = 39370534
```

**Why this works:**
- At 48kHz, we call `ProcessSample()` 48,000 times per second
- Each call adds 39370534 to phase
- After 48000 calls: `phase += 39370534 * 48000 = ~1,889,945,632,000`
- This causes phase to wrap ~440 times (440 complete cycles = 440Hz!)

**Python verification:**
```python
phase = 0
increment = 39370534
wraps = 0

for _ in range(48000):  # 1 second
    phase += increment
    if phase >= 2**32:
        phase %= 2**32
        wraps += 1

print(f"Frequency: {wraps} Hz")  # Should print ~440 Hz
```

---

## The `ProcessSample()` Method - Line by Line

This method is called **48,000 times per second** to generate audio. Let's break it down step by step.

### STEP 1: Wavetable Position Selection (Lines 53-64)

```cpp
int32_t xKnob = KnobVal(Knob::X);
```

**Reads X knob position.** Returns a value from 0 to 4095 (12-bit ADC resolution).

**Python:** `x_knob = read_knob()  # 0-4095`

---

```cpp
int32_t wavetablePos_fp16 = (xKnob * 59 * 65536) / 4095;
```

**Maps knob position to wavetable index with fractional precision.**

**Breakdown:**
- `xKnob` ranges from 0 to 4095
- We want to map this to 0.0 to 59.0 (60 waveforms, indexed 0-59)
- Multiply by 65536 to use 16-bit fixed-point precision

**Python equivalent:**
```python
x_knob = 2048  # Example: knob at center
wavetable_pos = (x_knob * 59.0) / 4095  # = 29.5 (floating-point)

# Fixed-point version:
wavetable_pos_fp16 = (x_knob * 59 * 65536) // 4095  # Integer representing 29.5
```

**Example values:**
- `xKnob = 0` → `wavetablePos_fp16 = 0` (waveform 0.0)
- `xKnob = 2048` → `wavetablePos_fp16 = 1933312` (≈ 29.5 in Q16)
- `xKnob = 4095` → `wavetablePos_fp16 = 3866624` (≈ 59.0 in Q16)

---

```cpp
int32_t waveIndex1 = wavetablePos_fp16 >> 16;
int32_t waveFrac = wavetablePos_fp16 & 0xFFFF;
```

**Splits the fixed-point value into integer and fractional parts.**

**How it works:**
- `>> 16` (right shift) extracts the upper 16 bits (integer part)
- `& 0xFFFF` (bitwise AND) extracts the lower 16 bits (fractional part)

**Python equivalent:**
```python
wavetable_pos_fp16 = 1933312  # Represents 29.5

# Extract integer part (wave index)
wave_index_1 = wavetable_pos_fp16 >> 16  # = 29
# or: wave_index_1 = wavetable_pos_fp16 // 65536

# Extract fractional part (0-65535 representing 0.0-0.999...)
wave_frac = wavetable_pos_fp16 & 0xFFFF  # = 32768 (represents 0.5)
# or: wave_frac = wavetable_pos_fp16 % 65536
```

**Visual representation:**
```
wavetablePos_fp16 = 1933312 (binary: 0000 0000 0001 1101 | 1000 0000 0000 0000)
                                      └─ Integer (29) ─┘   └─ Fraction (0.5) ─┘
                                           waveIndex1              waveFrac
```

---

```cpp
int32_t waveIndex2 = (waveIndex1 + 1) % numWaveforms;
```

**Gets the next waveform index for interpolation.**

**Why?** When morphing from waveform 29 to 30, we need samples from both.

**Python:** `wave_index_2 = (wave_index_1 + 1) % 60  # Wraps 59 → 0`

---

### STEP 2: Sample Position Within Waveform (Lines 70-74)

```cpp
uint64_t pos = uint64_t(phase) * samplesPerWaveform;
```

**Calculates which sample to read from the 600-sample waveform.**

**Why 64-bit?** To prevent overflow:
- `phase` can be up to 2³² ≈ 4.3 billion
- `samplesPerWaveform` = 600
- Multiplication: 4.3B × 600 ≈ 2.6 trillion (needs 64 bits!)

**Python equivalent:**
```python
phase = 2147483648  # Halfway through cycle (2^31)
samples_per_waveform = 600

pos = phase * samples_per_waveform  # = 1,288,490,188,800
```

---

```cpp
uint32_t sampleIndex = pos >> 32;
uint32_t sampleFrac = (pos >> 16) & 0xFFFF;
```

**Extracts integer and fractional sample position.**

**How it works:**
- `pos` is effectively a 64-bit fixed-point number with 32 bits of fraction
- Upper 32 bits = integer sample index (0-599)
- Next 16 bits = fractional position for interpolation

**Python equivalent:**
```python
pos = 1288490188800  # From above

sample_index = pos >> 32  # = 300 (middle of waveform)
# or: sample_index = pos // (2**32)

sample_frac = (pos >> 16) & 0xFFFF  # = 0 (exactly on sample)
# or: sample_frac = (pos // 65536) % 65536
```

**Visual:**
```
pos (64-bit) = [...32 bits of integer...][...16 bits fraction...][...16 unused...]
                        sampleIndex              sampleFrac
```

---

### STEP 3: Read and Interpolate Samples (Lines 81-92)

```cpp
int32_t wave1_s1 = wavetable[waveIndex1][sampleIndex];
int32_t wave1_s2 = wavetable[waveIndex1][(sampleIndex + 1) % samplesPerWaveform];
```

**Reads two adjacent samples from first waveform.**

**Why two samples?** To smoothly interpolate between them (prevents aliasing/stepping artifacts).

**Python equivalent:**
```python
wave_index_1 = 29
sample_index = 300

wave1_s1 = wavetable[wave_index_1, sample_index]       # Sample at index 300
wave1_s2 = wavetable[wave_index_1, (sample_index + 1) % 600]  # Sample at index 301
```

---

```cpp
int32_t wave1_sample = (wave2_s2 * sampleFrac + wave1_s1 * (65536 - sampleFrac)) >> 16;
```

**Linear interpolation between the two samples.**

**Formula:** `result = s1 + (s2 - s1) * fraction`

**Rewritten for integer math:**
```
result = s1 * (1 - fraction) + s2 * fraction
       = s1 * (65536 - sampleFrac) + s2 * sampleFrac  [multiply by 65536]
       = (s1 * (65536 - sampleFrac) + s2 * sampleFrac) >> 16  [divide by 65536]
```

**Python equivalent:**
```python
# Floating-point version (what you'd naturally write):
wave1_sample = wave1_s1 + (wave1_s2 - wave1_s1) * (sample_frac / 65536)

# Fixed-point version (what C++ code does):
wave1_sample = (wave1_s2 * sample_frac + wave1_s1 * (65536 - sample_frac)) >> 16
```

**Example:**
- If `sampleFrac = 0` (exactly on s1): result = `s1 * 65536 >> 16` = s1 ✓
- If `sampleFrac = 65535` (almost at s2): result ≈ s2 ✓
- If `sampleFrac = 32768` (halfway): result = `(s1 + s2) / 2` ✓

The same interpolation is done for the second waveform (lines 88-92).

---

### STEP 4: Interpolate Between Waveforms (Lines 102)

```cpp
int32_t interpolated = (wave2_sample * waveFrac + wave1_sample * (65536 - waveFrac)) >> 16;
```

**Blends the two waveforms together.**

This is the **wavetable morphing magic**! Uses the same linear interpolation as before, but now between two different waveforms instead of two samples.

**Python equivalent:**
```python
# If X knob is at 29.3, we're 30% of the way from wave 29 to wave 30
wave_frac = int(0.3 * 65536)  # = 19660

# Blend: 70% wave1 + 30% wave2
interpolated = (wave2_sample * wave_frac + wave1_sample * (65536 - wave_frac)) >> 16
```

**Result:** Smooth morphing as you turn the X knob!

---

```cpp
int32_t out = interpolated >> 4;
```

**Scales from 16-bit to 12-bit for DAC output.**

**Why?**
- Wavetable data is 16-bit (±32768)
- Audio DAC is 12-bit (±2048)
- `>> 4` divides by 16 (2⁴) to scale down

**Python:** `out = interpolated // 16`

---

### STEP 5: Pitch Control (Lines 116-142)

```cpp
int32_t knob = KnobVal(Knob::Main);
int32_t octaves_fp16 = (knob - 2048) * 64;
```

**Maps Main knob to ±2 octaves in fixed-point.**

**Breakdown:**
- Knob range: 0 to 4095
- Center: 2048
- Map to ±2 octaves: `(knob - 2048) * 64`
  - At knob = 0: octaves = -2.0 (2 octaves down)
  - At knob = 2048: octaves = 0.0 (reference pitch)
  - At knob = 4095: octaves = +2.0 (2 octaves up)

**Why multiply by 64?** To convert knob units to 16-bit fixed-point octaves:
```
4095 * 64 / 65536 = 4.0 octaves range (±2 octaves)
```

**Python equivalent:**
```python
knob = 2048  # Center position
octaves = ((knob - 2048) * 64) / 65536  # = 0.0 octaves
```

---

```cpp
int32_t octave_int = octaves_fp16 >> 16;
int32_t octave_frac = octaves_fp16 & 0xFFFF;
```

**Splits octaves into integer and fractional parts** (same technique as before).

**Example:**
- Knob at 3072 (75% of range)
- `octaves_fp16 = (3072 - 2048) * 64 = 65536` (represents 1.0 octave)
- `octave_int = 1`, `octave_frac = 0`

---

```cpp
if (octave_int >= 0) {
    phaseInc <<= octave_int;
} else {
    phaseInc >>= (-octave_int);
}
```

**Applies integer octave shifts via bit shifting.**

**Why this works:**
- Each octave up = double the frequency = multiply by 2
- Each octave down = halve the frequency = divide by 2
- Bit shifting is multiplication/division by powers of 2!

**Python equivalent:**
```python
if octave_int >= 0:
    phase_inc = base_phase_inc * (2 ** octave_int)  # Octaves up
else:
    phase_inc = base_phase_inc // (2 ** (-octave_int))  # Octaves down
```

**Example:**
- `basePhaseInc = 39370534` (440 Hz)
- `octave_int = 1` → `phaseInc = 78741068` (880 Hz) ✓
- `octave_int = -1` → `phaseInc = 19685267` (220 Hz) ✓

---

```cpp
int64_t fracAdjust = (int64_t(phaseInc) * octave_frac * 45426) >> 32;
phaseInc += fracAdjust;
```

**Applies fractional octave shift using linear approximation.**

**Math behind it:**
- To go up by `x` octaves: multiply frequency by `2^x`
- For small `x`: `2^x ≈ 1 + 0.693 * x` (linear approximation)
- In fixed-point: `2^x ≈ 1 + (45426/65536) * x` where 45426 ≈ 0.693 × 65536

**Python equivalent:**
```python
# If we want to go up 0.5 octaves (octave_frac = 32768):
frac_multiplier = 1 + 0.693 * (octave_frac / 65536)
phase_inc *= frac_multiplier
```

This gives smooth, musical pitch control!

---

```cpp
phase += phaseInc;
```

**Advances the phase accumulator.** This is the "heartbeat" of the synthesizer - incrementing phase creates oscillation.

**What happens:**
- Phase increases each sample
- When phase overflows (exceeds 2³²), it wraps back to 0
- This creates a repeating waveform at the desired frequency

---

## Performance Considerations

### Why Integer Math?

The RP2040 has **no hardware floating-point unit**. Floating-point operations are emulated in software and are ~100x slower.

**Timing budget at 48kHz:**
- Sample period: 1/48000 = 20.8 microseconds
- At 125MHz: ~2,600 CPU cycles per sample
- At 200MHz: ~4,200 CPU cycles per sample

**This code's operations:**
- Array lookups: ~1-2 cycles each
- Integer multiply: ~1 cycle
- Bit shifts: ~1 cycle
- Floating-point multiply: ~100+ cycles

By using integer math exclusively, this synthesizer comfortably fits in the timing budget!

### Memory Layout

```python
# Python visualization of memory usage:
wavetable_size = 60 * 600 * 2  # 72,000 bytes = 72 KB
rp2040_ram = 264 * 1024        # 264 KB total

usage_percent = (wavetable_size / rp2040_ram) * 100  # = 27%
```

The wavetable fits comfortably in RAM with plenty of room for other data structures.

---

## Summary: The Algorithm in Pseudocode

```python
# Initialization
phase = 0
base_phase_inc = 39370534  # 440 Hz

# Called 48,000 times per second
def process_sample():
    # 1. Read knobs
    x_knob = read_knob_x()      # 0-4095
    main_knob = read_knob_main()  # 0-4095

    # 2. Map X knob to wavetable position (0.0 to 59.0)
    wave_pos = (x_knob * 59.0) / 4095
    wave_idx_1 = int(wave_pos)
    wave_idx_2 = (wave_idx_1 + 1) % 60
    wave_frac = wave_pos - wave_idx_1

    # 3. Map phase to sample position (0 to 599)
    sample_pos = (phase / (2**32)) * 600
    sample_idx = int(sample_pos)
    sample_frac = sample_pos - sample_idx

    # 4. Read 4 samples (2 from each waveform)
    w1_s1 = wavetable[wave_idx_1, sample_idx]
    w1_s2 = wavetable[wave_idx_1, (sample_idx + 1) % 600]
    w2_s1 = wavetable[wave_idx_2, sample_idx]
    w2_s2 = wavetable[wave_idx_2, (sample_idx + 1) % 600]

    # 5. Interpolate within each waveform
    wave1_sample = lerp(w1_s1, w1_s2, sample_frac)
    wave2_sample = lerp(w2_s1, w2_s2, sample_frac)

    # 6. Interpolate between waveforms
    output = lerp(wave1_sample, wave2_sample, wave_frac)

    # 7. Scale to 12-bit and output
    output_12bit = output // 16
    audio_out(output_12bit)

    # 8. Calculate pitch from Main knob
    octaves = ((main_knob - 2048) / 2048) * 2  # ±2 octaves
    frequency_multiplier = 2 ** octaves
    phase_inc = base_phase_inc * frequency_multiplier

    # 9. Advance phase
    phase = (phase + phase_inc) % (2**32)

def lerp(a, b, t):
    """Linear interpolation: a + (b - a) * t"""
    return a + (b - a) * t
```

---

## Key Takeaways for Python Developers

1. **Fixed-point arithmetic** is like working with integers that represent decimals - multiply by 65536 to store, shift right by 16 to retrieve

2. **Bit shifting** (`<<`, `>>`) is fast integer multiplication/division by powers of 2

3. **Type declarations** (`int32_t`, `uint32_t`) specify exact sizes - important for embedded systems with limited memory

4. **No dynamic allocation** - everything is fixed-size arrays and stack variables (no `malloc()` or `new`)

5. **Real-time constraints** - code must complete in <20 microseconds per call (no `print()` debugging!)

6. **Linear interpolation** smooths discrete samples into continuous waveforms

7. **2D interpolation** (bilinear) allows morphing between waveforms

---

## Further Exploration Ideas

Want to extend this code? Try:

1. **Add Y knob control** for filter cutoff or resonance
2. **MIDI note input** to control pitch via CV input
3. **LFO modulation** to automatically sweep through waveforms
4. **Envelope follower** to modulate timbre with input amplitude
5. **Add more waveforms** (limit: ~3.6MB of wavetables before running out of RAM)
6. **Implement crossfading** for glitch-free waveform changes

---

## Resources

- **RP2040 Datasheet**: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
- **Fixed-Point Arithmetic**: https://en.wikipedia.org/wiki/Fixed-point_arithmetic
- **Wavetable Synthesis**: https://en.wikipedia.org/wiki/Wavetable_synthesis
- **ComputerCard Documentation**: See `ComputerCard.h` and examples in `../examples/`

---

**Questions?** The code is heavily commented - refer to inline comments for specific details!
