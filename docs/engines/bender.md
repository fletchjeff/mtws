# bender

## Goal + Sonic Intent
A waveshaping oscillator with two modes: folded tri/sine and triangle-to-ramp morph.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: fold amount (normal) / morph amount (alt)
- Y: bias/offset (normal) / out2 phase shift (alt)
- Z Up: Alt (triangle->ramp)
- Z Middle: Normal (wavefolder)
- Z Down: ignored in standalone
- Out1: triangle/fold path or ramp-up morph
- Out2: sine/fold path or ramp-down morph

## DSP Block Diagram
- Normal: triangle + sine sources -> drive/bias -> folding function
- Alt: triangle source -> ramp morph on out1; phase-shifted triangle -> ramp-down morph on out2

## CPU-Risk Points
- Fold loops on large drive values
- Per-sample LUT read for sine output

## Milestone Steps
1. Tune fold drive range for musical sweet spots.
2. Add anti-aliased ramp morph path if needed.
3. Add mode-matched output leveling.

## Hardware Test Checklist
- Verify fold symmetry with centered bias.
- Verify Alt out2 phase shift tracks Y smoothly.
- Check no zipper noise on fast X/Y sweeps.
