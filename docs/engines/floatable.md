# floatable

## Goal + Sonic Intent
Wavetable oscillator with 8x8 table addressing and bank switch in Alt mode.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: horizontal wavetable interpolation between adjacent columns (or AudioIn1 attenuator)
- Y: vertical wavetable interpolation between adjacent rows (or AudioIn2 attenuator)
- Z Up: Alt bank select
- Z Middle: Normal bank select
- Z Down: ignored in standalone
- Out1: forward morph output
- Out2: inverse-X morph output over the same Y position

## DSP Block Diagram
- Select the 2x2 wavetable neighborhood around the current X/Y position
- Interpolate samples in-wave for all four corner tables
- Bilinearly morph across X and Y
- Out2 reuses the same corner samples with inverse X direction

## CPU-Risk Points
- Per-sample bilinear wavetable interpolation
- High-frequency aliasing from bright waves

## Milestone Steps
1. Verify both banks with current 64x512 data.
2. Reintroduce optional anti-alias table strategy later.
3. Add web tooling for curated table set updates.

## Hardware Test Checklist
- Check smooth X/Y movement across the full grid, especially row boundaries.
- Confirm Alt switches bank audibly.
- Verify stereo relation remains stable.
