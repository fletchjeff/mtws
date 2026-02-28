Building an additive synthesizer for the Music Thing Workshop System (which runs on the RP2040) is an awesome project! However, hitting processing limits with additive synthesis is basically a rite of passage.

The RP2040 uses a Cortex-M0+ processor. It's fantastic, but it has two major hardware constraints that your current `main.cpp` is fighting against in the inner audio loop:

1. **No 64-bit hardware multiplier:** Any `int64_t` or `uint64_t` math triggers a slow software library function (`__aeabi_lmul`).
2. **No hardware division instruction:** The RP2040 has a hardware divider peripheral, but it still takes ~15-30 cycles, which adds up fast if you use it heavily per-sample.

Since your `ProcessSample()` runs 48,000 times a second and loops over 12 partials, every cycle counts. Here are the most impactful optimizations you can make to your implementation.

---

### 1. Eradicate 64-bit Math in the Inner Loop

You are currently casting to `int64_t` and `uint64_t` several times per partial to avoid overflow during Q-math operations. We can shift values down slightly *before* multiplying so everything stays comfortably inside native 32-bit integers.

**Instead of this:**

```cpp
sum += int32_t((int64_t(contrib) * alias_w_q16) >> 16);
gain_sum += int32_t((int64_t(gain_q12) * alias_w_q16) >> 16);

```

**Shift the alias weight to Q7 or Q8 to stay in 32-bit bounds:**

```cpp
// Shift weight down from Q16 (0..65536) to Q7 (0..128)
uint32_t alias_w_q7 = alias_w_q16 >> 9; 

// Max contrib is ~8.3M. 8.3M * 128 = ~1.07 Billion (fits inside max int32_t of 2.14B)
sum += (contrib * alias_w_q7) >> 7; 
gain_sum += (gain_q12 * alias_w_q7) >> 7; 

```

### 2. Nuke the Division in `CleanAliasWeightQ16`

Inside your anti-aliasing function, you are dividing by `span` (which is a compile-time constant of `120,795,956`). Because this happens inside the `PARTIALS` loop, you are doing 12 expensive divisions per sample.

You can replace this division with a 32-bit reciprocal multiplication.

**Instead of this:**

```cpp
uint32_t span = CUTOFF_PHASE_INC_90PCT_NYQUIST - CUTOFF_PHASE_INC_FADE_START;
uint32_t rem = CUTOFF_PHASE_INC_90PCT_NYQUIST - partial_inc;
return uint32_t((uint64_t(rem) << 16) / span);

```

**Use an approximate fractional multiplier:**

```cpp
uint32_t rem = CUTOFF_PHASE_INC_90PCT_NYQUIST - partial_inc;
// (rem * 65536) / 120795956 is mathematically equivalent to rem * 0.000542...
// We can approximate this in 32-bit integer math: (rem * 36) >> 16
// Max rem is ~120M. 120M * 36 = 4.3 Billion (fits just inside uint32_t)
return (rem * 36) >> 16; 

```

### 3. Optimize the Sine Table Interpolation

Your `LookupSine` function does two multiplications. We can mathematically simplify the linear interpolation to use only a single multiplication, shaving off a few more cycles.

**Instead of this:**

```cpp
return int16_t((s2 * frac + s1 * (65536 - frac)) >> 16);

```

**Factor it to one multiplication:**

```cpp
return int16_t(s1 + (((s2 - s1) * (int32_t)frac) >> 16));

```

### 4. Decimate Control-Rate Calculations

Right now, you are reading `KnobVal()`, computing `PitchToPhaseInc()`, and calculating `MorphQ16FromX()` 48,000 times a second. Hardware knobs and smoothed CV don't change fast enough to require audio-rate updates.

You can process these "Control Rate" variables every 16 or 32 samples (decimation).

**How to implement:**

1. Add a simple counter variable to your class: `int control_tick = 0;`
2. At the top of `ProcessSample()`, wrap your knob reads and pitch math in an `if` statement:

```cpp
if (++control_tick >= 16) {
    control_tick = 0;
    // Read knobs, update pitch_smoothed, phase_inc, and morph_q16 here
}

```

*Note: Because `y` is also read at control rate, you can pre-calculate the 12 values for `gain_q12` inside this decimation block and store them in an array, entirely removing `PartialGainQ12(i, y)` from the inner audio loop!*
