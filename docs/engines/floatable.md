# floatable

## Goal + Sonic Intent
Stereo wavetable oscillator with an 8x8 morph surface, bank switch in Alt mode,
and an inverse-X companion output for width.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: horizontal wavetable interpolation between adjacent columns (or AudioIn1 attenuator)
- Y: vertical wavetable interpolation between adjacent rows (or AudioIn2 attenuator)
- Z Up: Alt bank select
- Z Middle: Normal bank select
- Z Down: ignored in standalone
- Out1: forward bilinear morph output
- Out2: inverse-X morph output over the same Y position

## DSP Block Diagram
- Control: map X/Y across a non-wrapping 8x8 surface, select the 2x2 wavetable neighborhood, and store Q12 X/Y fractions
- Audio: phase-interpolate the 4 corner waves at the current phase
- Out1: X-lerp top and bottom rows, then Y-lerp those row results
- Out2: reuse the same corner samples and row results with inverse X over the same Y position

## CPU-Risk Points
- 4 in-wave sample interpolations per output sample
- Bilinear X/Y wavetable interpolation on every audio sample
- High-frequency aliasing from bright waves

## Milestone Steps
1. Curate stronger bank content while preserving neighborhood continuity across the 8x8 surface.
2. Re-evaluate whether inverse-X `Out2` remains the final stereo strategy once the curated banks are in place.
3. Reintroduce optional anti-alias table strategy later only if the final bank set needs it.

## Hardware Test Checklist
- Check smooth X/Y movement across the full grid, especially row boundaries.
- Confirm Alt switches bank audibly.
- Verify stereo relation remains stable and useful.
- Sweep X slowly and listen for any remaining `Out2` boundary clicks on strong table transitions.

## Implementation Notes
- Integrated `mtws` currently uses the original `64 x 512` bank A/B source tables with direct audio-rate bilinear interpolation in [floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp).
- The standalone `floatable` target is currently a separate `4 x 64 x 256` rendered-table sandbox for bank-routing and wavetable-set experiments; see [WAVETABLE.md](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/WAVETABLE.md).
