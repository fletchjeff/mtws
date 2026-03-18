# Standalone floatable wavetable notes

## Purpose

The standalone `floatable` target is now the same curated-bank implementation as
integrated `mtws`, not a separate bank-routing sandbox.

## Active source banks

Both builds share these four generated `16 x 256` bank headers:

- `knots/src/wavetables/floatable_bank_1_16x256.h`
- `knots/src/wavetables/floatable_bank_2_16x256.h`
- `knots/src/wavetables/floatable_bank_3_16x256.h`
- `knots/src/wavetables/floatable_bank_4_16x256.h`

They were generated from these browser exports:

- `tools/floatable_wavetable_browser/1_floatable_selection_16_b.json`
- `tools/floatable_wavetable_browser/2_floatable_selection_16.json`
- `tools/floatable_wavetable_browser/3_floatable_selection_16_fm.json`
- `tools/floatable_wavetable_browser/4_floatable_selection_16_snippets.json`

## Runtime mapping

- `Alt = false`: `Out1 <- bank 1`, `Out2 <- bank 2`
- `Alt = true`: `Out1 <- bank 3`, `Out2 <- bank 4`
- `X`: morphs across the `16` waves in the current Out1 bank
- `Y`: morphs across the `16` waves in the current Out2 bank

## Render architecture

The standalone engine in `knots/solo_engines/floatable/floatable_solo_engine.cpp` works in two stages:

1. Control core renders two finished `256`-sample tables:
   - `rendered_out1[256]`
   - `rendered_out2[256]`
2. Audio core phase-interpolates those finished tables with one shared phasor.

### Why `256` samples

`256` was chosen because it is the stable table size already proven in the solo
build.

- What it represents: one full single-cycle table length for each rendered output
- Why it was chosen: it keeps the render-frame footprint small enough to avoid the earlier stack-pressure failure while still sounding stable
- Alternative considered: `512` rendered samples per output
- CPU vs memory tradeoff: `256` halves rendered-table memory and read bandwidth versus `512`, at the cost of lower phase-domain resolution

### Audio-rate phase mapping

- Formula in plain language: the top `8` phase bits choose one of `256`
  samples and the next `12` bits blend toward the next sample
- Endpoint behavior: the sample index wraps cleanly from `255` back to `0`
- Quantization behavior: table lookup is quantized to `256` stored points, with
  Q12 interpolation smoothing the fencepost between adjacent samples

## Legacy files

The older `64 x 256` and `64 x 512` floatable headers in this folder are now
reference-only and are not part of the active runtime path.
