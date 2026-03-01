# floatable

## Goal + Sonic Intent
Wavetable oscillator with 8x8 table addressing and bank switch in Alt mode.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: horizontal wavetable interpolation (or AudioIn1 attenuator)
- Y: wavetable row select (or AudioIn2 attenuator)
- Z Up: Alt bank select
- Z Middle: Normal bank select
- Z Down: ignored in standalone
- Out1: forward morph output
- Out2: inverse morph output

## DSP Block Diagram
- Select 8x8 table cell from Y and adjacent X pair
- Interpolate samples in-wave and across X pair
- Out2 uses inverse interpolation direction

## CPU-Risk Points
- Per-sample table interpolation
- High-frequency aliasing from bright waves

## Milestone Steps
1. Verify both banks with current 64x512 data.
2. Reintroduce optional anti-alias table strategy later.
3. Add web tooling for curated table set updates.

## Hardware Test Checklist
- Check X/Y movement across full grid.
- Confirm Alt switches bank audibly.
- Verify stereo relation remains stable.
