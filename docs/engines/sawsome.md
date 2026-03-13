# sawsome

## Goal + Sonic Intent
A supersaw / supertriangle oscillator scaffold with 5 detuned voices, fixed stereo pan map, and JP-style spread behavior.

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
- Normal: 5 x detuned saw (polyBLEP edge correction) -> per-voice pan/gain -> stereo sum
- Alt: same voice map, waveform switched to leaky-integrated polyBLEP triangle path

## CPU-Risk Points
- Per-voice BLEP edge checks on 5 oscillators
- Stereo voice summing

## Milestone Steps
1. Lock detune/pan/gain constants by ear on hardware.
2. Add better anti-aliasing at high pitch if needed.
3. Add optional phase randomization on mode entry.

## Hardware Test Checklist
- Verify width and detune both reach expected extremes.
- Confirm Alt clearly differs from Normal.
- Confirm no hard clipping when both controls full CW.
