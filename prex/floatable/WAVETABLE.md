# Standalone floatable wavetable notes

## Purpose

This folder is now a standalone sandbox for `floatable` wavetable architecture
and bank-curation experiments. It is intentionally separate from the integrated
`mtws` engine documentation in [docs/engines/floatable.md](/Users/jeff/Toonbox/MTWS/mtws/docs/engines/floatable.md).

The current solo build is not the old `60 x 600` wavetable prototype anymore.
That older material remains in this folder as legacy reference data only.

## Current standalone implementation

### Source bank layout

The active solo build uses four local `64 x 256` source banks:

- [floatable_wavetable_1_reversed_64x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_1_reversed_64x256.h)
- [floatable_wavetable_2_original_64x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_2_original_64x256.h)
- [floatable_wavetable_3_original_64x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_3_original_64x256.h)
- [floatable_wavetable_4_reversed_64x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_4_reversed_64x256.h)

The current test arrangement is deliberate:

- bank `1` == bank `4` : reversed source
- bank `2` == bank `3` : original source

That makes `alt` an audible routing check instead of a new-content test.

### Render architecture

The standalone engine in [floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp) works in two stages:

1. Control core builds two final `256`-sample rendered tables:
   - `rendered_out1[256]`
   - `rendered_out2[256]`
2. Audio core phase-interpolates those finished tables with one shared phasor.

This is a rendered-table testbed. It is not the same architecture as the
integrated `mtws` floatable engine, which still reads directly from the source
`64 x 512` banks at audio rate.

### Control behavior

- `Main`: pitch
- `X`: source-bank interpolation position for `Out1`
- `Y`: source-bank interpolation position for `Out2`
- `Alt = false`: `Out1 <- bank 1`, `Out2 <- bank 2`
- `Alt = true`: `Out1 <- bank 3`, `Out2 <- bank 4`

Because `1 == 4` and `2 == 3`, flipping `alt` swaps the left/right source
assignment while keeping the underlying source content the same.

## Why the rendered tables are `256` samples

The standalone engine originally tried `2 x 512` rendered output tables and
crashed immediately on load. The issue was not general SRAM exhaustion; it was
core-0 stack pressure from putting a larger `SoloAppBase<FloatableSoloEngine>`
object on the main stack.

`256` was chosen because:

- one `512` rendered table = `1024` bytes
- two `256` rendered tables = `1024` bytes

So `2 x 256` returns the render-frame footprint to the same class as the known
working single-`512` test, while still allowing a true two-output architecture.

### Audio-rate phase mapping

For the current `256`-sample rendered tables:

- top `8` phase bits select the sample index
- next `12` bits are the Q12 interpolation fraction

This keeps the audio-rate read path cheap while matching the reduced table
length.

## Files that are legacy or reference-only

These are not part of the current standalone runtime path:

- [wavetable_data.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/wavetable_data.h)
- [AKWF_60_ordered.wav](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/AKWF_60_ordered.wav)
- [floatable_wavetable_1_original_64x512.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_1_original_64x512.h)
- [floatable_wavetable_3_reversed_64x512.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_wavetable_3_reversed_64x512.h)

They are kept as source/reference material for future bank generation and
comparison, not as the current solo-engine data path.

## Current technical conclusion

The wavetable architecture is now proven technically in the standalone build:

- `2 x 256` rendered outputs are stable
- four local source banks load cleanly
- normal/alt bank routing works
- alt can swap audible source roles as expected

The next step is not engine debugging. It is curating the actual wavetable bank
content.
