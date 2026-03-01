# cumulus

## Goal + Sonic Intent
Additive oscillator with harmonic partials, movable centroid, and alt ratio warping.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: centroid position (or AudioIn1 attenuator)
- Y: gain spread / alt warp amount (or AudioIn2 attenuator)
- Z Up: Alt ratio warp
- Z Middle: Normal harmonic mode
- Z Down: ignored in standalone
- Out1: odd partial bus
- Out2: even partial bus

## DSP Block Diagram
- Control: per-partial ratio/gain targets -> smoothing -> phase increments + gains
- Audio: 16 sine partials -> odd/even sum -> fixed headroom -> auto-capped makeup gain

## CPU-Risk Points
- 16-voice render loop
- Control-rate per-partial smoothing and guard logic

## Milestone Steps
1. Tune master gain/noise-floor compromise.
2. Add optional Nyquist-aware partial drop/morph refinement.
3. Revisit makeup strategy if required by mix context.

## Hardware Test Checklist
- Validate centroid movement and symmetric gain bump.
- Validate alt collapse/repel behavior.
- Validate output level consistency vs other engines.
