# bender

## Goal + Sonic Intent
A waveshaping oscillator that compares Utility-Pair-style folding and crushing
paths from one shared sine source.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: fold drive and fold wet/dry mix
- Y: crush amount
- Z Up: Alt (stage reordering)
- Z Middle: Normal (fold vs crush comparison)
- Z Down: ignored in standalone
- Out1: dry/wet fold path in Normal, fold-then-crush in Alt
- Out2: crusher path in Normal, crush-then-fold in Alt

## DSP Block Diagram
- Shared source: sine LUT oscillator
- Normal: Out1 = sine -> fold drive -> antialiased folder -> dry/wet mix, Out2 = sine -> crusher
- Alt: Out1 = fold -> crusher, Out2 = crusher -> fold

## CPU-Risk Points
- Antialiased fold path uses an antiderivative state machine
- Alt mode runs both nonlinear stages on both outputs
- Per-sample LUT read for the shared sine source

## Milestone Steps
1. Tune fold drive range for musical sweet spots.
2. Check whether the fold-vs-crush level relationship needs additional mode balancing.
3. Add optional drive or crush curves later only if the current linear/squared maps feel too abrupt.

## Hardware Test Checklist
- Verify X moves smoothly from mostly dry sine to fully folded output.
- Verify Y stays subtle at low settings and gets aggressively coarse near full CW.
- Confirm Alt clearly changes the output by reordering the nonlinear stages.
