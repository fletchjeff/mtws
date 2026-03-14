# sawsome

## Goal + Sonic Intent
A supersaw / supertriangle oscillator with 7 detuned voices, fixed stereo pan,
and level-compensated spread so detune sweeps stay musically consistent.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: stereo width (or AudioIn1 attenuator when connected)
- Y: detune depth (or AudioIn2 attenuator when connected)
- Z Up: Alt (supertriangle)
- Z Middle: Normal (supersaw)
- Z Down: ignored in standalone
- Out1: left mix
- Out2: right mix

## DSP Block Diagram
- Control: detune and width -> per-voice phase increments + makeup gain + stereo pan
- Normal: 7 x detuned PolyBLEP saws -> per-voice pan/gain -> stereo sum
- Alt: same 7-voice spread, but each saw is leaky-integrated into a triangle path

## CPU-Risk Points
- Per-voice BLEP edge checks on 7 oscillators
- Stereo voice summing and per-voice gain multiplies

## Milestone Steps
1. Lock detune/pan/gain constants by ear on hardware.
2. Check that center-only to full-spread level movement feels controlled.
3. Add optional phase randomization on mode entry if the current fixed seeds feel too static.

## Hardware Test Checklist
- Verify detune fully collapses to the center voice at full CCW.
- Verify width and detune both reach expected extremes.
- Confirm Alt clearly differs from Normal.
- Confirm no hard clipping when both controls full CW.
