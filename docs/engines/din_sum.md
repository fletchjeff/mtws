# din_sum

## Goal + Sonic Intent
Filtered-noise oscillator scaffold with pitch-centered split filters and mode-dependent topology.

## Control Map
- Main: pitch reference (10Hz..10kHz)
- X: shared filter Q/damping (or AudioIn1 attenuator)
- Y: distance from center frequency (or AudioIn2 attenuator)
- Z Up: Alt (pink-ish source + HP/LP split)
- Z Middle: Normal (white source + BP/BP split)
- Z Down: ignored in standalone
- Out1: filter path 1
- Out2: filter path 2

## DSP Block Diagram
- Noise source (white or pink-ish)
- Normal: two SVF bandpass filters at low/high offsets
- Alt: SVF highpass + SVF lowpass

## CPU-Risk Points
- Multiple SVF updates per sample
- Noise generation + filter state updates at audio rate

## Milestone Steps
1. Tune bandwidth response vs X for stable musical range.
2. Tune center-distance response vs Y.
3. Improve pink-noise approximation if needed.

## Hardware Test Checklist
- Confirm overlap->split behavior across Y sweep.
- Confirm Alt HP/LP separation.
- Check for filter instability at extreme settings.
