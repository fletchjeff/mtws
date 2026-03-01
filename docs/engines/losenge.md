# losenge

## Goal + Sonic Intent
Vowel-style synth scaffold using a pitched carrier into dual formant bandpass pairs.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: Out1 formant morph (or AudioIn1 attenuator)
- Y: Out2 formant morph (or AudioIn2 attenuator)
- Z Up: Alt (square carrier + alt vowel pairs)
- Z Middle: Normal (saw carrier + base vowel pairs)
- Z Down: ignored in standalone
- Out1: first vowel morph path
- Out2: second vowel morph path

## DSP Block Diagram
- Carrier oscillator (saw/square)
- Two SVF bandpass filters per output (parallel)
- Output = average of each output's filter pair

## CPU-Risk Points
- Four SVF updates per sample
- Formant coefficient movement at control rate

## Milestone Steps
1. Tune vowel formant frequency sets by ear.
2. Add optional dynamic Q mapping per mode.
3. Add anti-aliasing on square carrier as needed.

## Hardware Test Checklist
- Confirm independent morph paths on out1/out2.
- Confirm alt vowel set is clearly distinct.
- Check high-pitch stability of formant tracking.
