Here are the key optimizations, ordered roughly by impact on the hot `ProcessSample` path:

## 1. Move control-rate work out of the per-sample loop

The knobs only update every ~4 samples (mux cycling), yet you recompute morph, harmonic ratios, partial gains, and alias weights every sample. Decimating this to every N samples is the single biggest win:

```cpp
// Add members:
uint32_t control_counter = 0;
static constexpr int CONTROL_DIVISOR = 16; // ~3kHz control rate
uint32_t cached_partial_inc[PARTIALS];
int32_t  cached_gain_q12[PARTIALS];
uint32_t cached_alias_w_q16[PARTIALS];
```

Then in `ProcessSample`, only recompute the per-partial arrays every N samples, and the inner loop just does phase accumulation + lookup + multiply.

## 2. Eliminate divisions — especially the per-sample normalization

The Cortex-M0+ has no hardware divider, so every `/` is an expensive software call.

**The `/ gain_sum` normalization every sample is the worst offender.** Precompute the reciprocal at control rate:

```cpp
// At control rate, precompute:
// inv_gain_q16 ≈ (UNITY_Q12 << 16) / gain_sum
uint32_t inv_gain_q16 = 0;
if (gain_sum > 0)
    inv_gain_q16 = (uint32_t)((uint64_t(UNITY_Q12) << 16) / gain_sum);

// Then per sample, the normalization is just a multiply:
int32_t out = (int32_t)((int64_t(sum) * inv_gain_q16) >> 16);
```

**`/ 4095U` in `PitchToPhaseInc`** — replace with multiply-by-reciprocal:

```cpp
// Instead of: (pitch_control * (8U << 16)) / 4095U
// Use: pitch_control * 128, then multiply by (2^32 / 4095) ≈ 1048961 and shift
uint32_t octaves_fp16 = (uint32_t)((uint64_t(pitch_control) * ((8ULL << 32) / 4095U + 1)) >> 16);
```

**`/ den` in `MorphQ16FromX`** — since this only runs at control rate after optimization #1, it matters less, but you could precompute the reciprocal or use a fixed scaling since the deadband values are constants (meaning `den` is constant: `2048 - 80 - 80 = 1888`).

## 3. Remove dead code

`grit_mode` is hardcoded `false` — the alias-bypass path is never taken. Remove the variable and simplify:

```cpp
// Before (in loop):
uint32_t alias_w_q16 = grit_mode ? 65536 : CleanAliasWeightQ16(partial_inc);

// After:
uint32_t alias_w_q16 = CleanAliasWeightQ16(partial_inc);
```

Or better yet, with optimization #1, the alias weight is precomputed and this disappears from the inner loop entirely.

## 4. Reduce 64-bit operations in the inner loop

After applying #1, the inner audio-rate loop becomes minimal, but you still have the phase accumulation and sine lookup. For `LookupSine`, consider **8-bit fractional interpolation** instead of 16-bit — the table only has 256 entries with 11-bit amplitude, so 8-bit interpolation is more than sufficient:

```cpp
inline int16_t LookupSine(uint32_t p)
{
    uint32_t idx = (p >> 24) & TABLE_MASK;
    uint32_t frac = (p >> 16) & 0xFF; // 8-bit fraction
    int32_t s1 = sine_table[idx];
    int32_t s2 = sine_table[(idx + 1) & TABLE_MASK];
    return (int16_t)(s1 + (((s2 - s1) * (int32_t)frac) >> 8));
}

```

This avoids the 32×32→64 implicit promotion and keeps everything in 32-bit arithmetic.

## 5. Increase sine table size to skip interpolation entirely

Alternatively, a 2048-entry table with no interpolation is often faster than a 256-entry table with interpolation on M0+. At `int16_t`, that's 4KB — easily fits in SRAM. The lookup becomes a single array read with zero multiplies.

## 6. Precompute the pitch-smoothed phase increment

`pitch_smoothed` changes very slowly (1-pole IIR with α=1/32). You could compute `PitchToPhaseInc` at control rate and interpolate the phase increment linearly between control updates, avoiding the exp2 lookup table and 64-bit multiply at audio rate entirely.

## Summary: optimized inner loop

After all these changes, your per-sample hot path shrinks to roughly:

```cpp
void ProcessSample() override
{
    // Smooth pitch (cheap, keep at audio rate)
    int32_t pitch = KnobVal(Knob::Main) + (CVIn1() >> 1);
    pitch = clamp(pitch, 0, 4095);
    pitch_smoothed = (31 * pitch_smoothed + (pitch << 4)) >> 5;

    // Control-rate updates
    if (++control_counter >= CONTROL_DIVISOR) {
        control_counter = 0;
        UpdateControlRate(); // recompute phase_incs, gains, alias, inv_gain
    }

    // Audio-rate: just accumulate + lookup + weighted sum
    int32_t sum = 0;
    for (int i = 0; i < PARTIALS; ++i) {
        phases[i] += cached_partial_inc[i];
        int32_t s = LookupSine(phases[i]); // fast 8-bit interp or table-only
        sum += (s * cached_gain_q12[i]) >> 12; // already alias-weighted
    }

    int32_t out = (int32_t)((int64_t(sum) * inv_gain_q16) >> 16);
    out = clamp(out, -2048, 2047);
    AudioOut1(out);
    AudioOut2(out);
}
```

This cuts the per-sample work from ~12 divisions + ~36 64-bit multiplies down to ~12 table lookups + ~12 32-bit multiplies + 1 64-bit multiply. On an M0+ at 48kHz, that's a substantial difference — probably 3–5× faster in the inner loop.