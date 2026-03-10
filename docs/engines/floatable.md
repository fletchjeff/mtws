# floatable

## Goal + Sonic Intent
Stereo wavetable oscillator with one curated wavetable lane per output and a
bank-pair switch in Alt mode.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: wavetable position inside the current Out1 bank (or AudioIn1 attenuator)
- Y: wavetable position inside the current Out2 bank (or AudioIn2 attenuator)
- Z Up: Alt bank select
- Z Middle: Normal bank select
- Z Down: ignored in standalone
- Out1: rendered output from the selected Out1 bank
- Out2: rendered output from the selected Out2 bank

## DSP Block Diagram
- Control: choose the active bank pair, then render one finished 256-sample table for Out1 from X and one finished 256-sample table for Out2 from Y
- Audio: phase-interpolate each rendered 256-sample table at the current oscillator phase
- Normal: Out1 <- bank 1, Out2 <- bank 2
- Alt: Out1 <- bank 3, Out2 <- bank 4

## CPU-Risk Points
- Rebuilding two 256-sample rendered tables every control tick
- Shared control frame is larger because it carries both rendered tables
- High-frequency aliasing from bright waves

## Milestone Steps
1. Hardware-check the new curated bank order in both standalone and integrated builds.
2. Reorder waves only if X or Y sweeps expose weak transitions inside a bank.
3. Reintroduce optional anti-alias table strategy later only if the final bank set needs it.

## Hardware Test Checklist
- Check smooth X movement across the full Out1 bank and Y movement across the full Out2 bank.
- Confirm Alt switches bank audibly.
- Verify the normal pair and alt pair both have useful stereo contrast.
- Sweep slowly through all 16 waves in each bank and listen for boundary clicks or weak order choices.

## Implementation Notes
- Integrated `mtws` and the standalone `floatable` target now share the same four curated `16 x 256` bank headers in [prex/mtws/wavetables](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables).
- Both builds now render two finished `256`-sample output tables at control rate and only phase-read those finished tables at audio rate; see [floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp) and [floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp).
