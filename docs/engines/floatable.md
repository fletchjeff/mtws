# floatable

## Goal + Sonic Intent
Stereo wavetable oscillator with one curated source bank per mode and
independent Out1/Out2 morph positions inside that shared timbral space.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: Out1 wavetable position inside the active source bank (or AudioIn1 attenuator)
- Y: Out2 wavetable position inside the active source bank (or AudioIn2 attenuator)
- Z Up: Alt bank select
- Z Middle: Normal bank select
- Z Down: ignored in standalone
- Out1: morph point chosen by X
- Out2: morph point chosen by Y

## DSP Block Diagram
- Control: choose the active source bank, then compute a source-wave index + Q12 fraction for X and Y
- Audio: phase-interpolate inside two adjacent source waves, then morph between those waves for each output
- Normal: both outputs read from curated bank 1
- Alt: both outputs read from curated bank 4

## CPU-Risk Points
- Two-stage interpolation on both outputs every sample
- Bright waves can still alias at high pitch

## Milestone Steps
1. Hardware-check the current curated bank order in both standalone and integrated builds.
2. Reorder waves only if X or Y sweeps expose weak transitions inside the bank.
3. Reintroduce an alternate anti-alias strategy only if the current direct-source approach proves too bright at extreme pitches.

## Hardware Test Checklist
- Check smooth X movement across the full Out1 path and Y movement across the full Out2 path.
- Confirm Alt switches bank audibly.
- Verify the two outputs remain meaningfully different even though they share the same active bank.
- Sweep slowly through all 16 waves in each active bank and listen for boundary clicks or weak order choices.

## Implementation Notes
- Integrated `mtws` and the standalone `floatable` target now share the same four curated `16 x 256` bank headers in [prex/mtws/wavetables](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables).
- Both builds now use the same compact morph-state approach and render directly from source waves at audio rate; see [floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp) and [floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp).
- The current curated mapping uses bank 1 in Normal and bank 4 in Alt. Banks 2 and 3 remain available in the repo but are not currently active in the engine.
