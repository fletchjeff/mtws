# additive_alias16

This folder preserves the 16-partial additive experiment that produces the intentional/interesting high-frequency grit.

## Why it sounds like aliasing / bit-crush

- Sample rate is 48 kHz, so Nyquist is 24 kHz.
- The oscillator generates harmonic partials 1..16 at `n * f0`.
- When `n * f0 > 24 kHz`, that partial folds back into the audible band (aliasing).
- Many folded partials create inharmonic sidebands that can resemble bit-crush or digital tearing.

Examples of where foldback starts:
- Partial 16 starts aliasing above `f0 = 1500 Hz`.
- Partial 12 starts aliasing above `f0 = 2000 Hz`.
- Partial 8 starts aliasing above `f0 = 3000 Hz`.

Since this patch can sweep up to 5120 Hz fundamental, the upper range is heavily aliased by design.

Preserved from `prex/additive/main.cpp` on 2026-02-17.
