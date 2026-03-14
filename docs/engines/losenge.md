# losenge

## Goal + Sonic Intent
Vowel-style formant oscillator inspired by Braids/Twists VOWL, with mirrored
stereo vowel motion and a brighter upper-formant alternate mode.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: vowel position for Out1; Out2 follows the inverse path (or AudioIn1 attenuator)
- Y: shared tract/formant shift (or AudioIn2 attenuator)
- Z Up: Alt (upper F2/F3/F4 formant set)
- Z Middle: Normal (base F1/F2/F3 formant set)
- Z Down: ignored in standalone
- Out1: forward vowel path
- Out2: inverse vowel path

## DSP Block Diagram
- Hidden carrier oscillator sets pitch and hard-resets the formant bank each cycle
- Per output: sine formant 1 + sine formant 2 + square formant 3 -> weighted sum -> glottal envelope
- Normal: uses the base F1/F2/F3 anchor table
- Alt: swaps to a brighter upper F2/F3/F4 anchor table

## CPU-Risk Points
- Six independent formant oscillator updates per sample
- Control-rate formant interpolation and Hz-to-phase conversion
- High-pitch stability of the hard-reset formant bank

## Milestone Steps
1. Tune vowel formant frequency sets by ear.
2. Check whether the upper-formant alt mode needs gain trimming versus the base path.
3. Add alternative formant sets later only if the current vowel path feels too narrow.

## Hardware Test Checklist
- Confirm X moves the two outputs in opposite vowel directions.
- Confirm Y brightens/darkens both outputs together around a neutral midpoint.
- Confirm Alt is clearly brighter and more metallic than Normal.
- Check high-pitch stability of formant tracking.
