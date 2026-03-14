# din_sum

## Goal + Sonic Intent
Bounded sine-to-saw morph oscillator with stochastic instability in Normal mode
and cycle-locked probabilistic waveform selection in Alt mode.

## Control Map
- Main: pitch (10Hz..10kHz)
- X: shared sine-to-saw morph position / saw probability (or AudioIn1 attenuator)
- Y: random-source coherence in Normal / cycle hold stickiness in Alt (or AudioIn2 attenuator)
- Z Up: Alt (cycle-locked probabilistic mode)
- Z Middle: Normal (bounded random morph mode)
- Z Down: ignored in standalone
- Out1: sine-to-local-saw morph
- Out2: sine-to-90-degree-shifted-saw morph

## DSP Block Diagram
- Shared sources: sine LUT oscillator plus two PolyBLEP saw targets
- Normal: X sets the center morph position; Y crossfades between smooth low-passed and nervous high-passed random modulation inside a bounded corridor
- Alt: each phase wrap chooses sine or saw for the whole cycle, independently per output, with X setting the saw probability and Y setting hold length

## CPU-Risk Points
- Two PolyBLEP saw evaluations per sample
- Audio-rate interpolation of the random corridor inside each 16-sample control block
- Alt-mode wrap detection and independent cycle rerolls

## Milestone Steps
1. Tune the coherence curve so the center region stays lively without losing pitch.
2. Check that the 90-degree Out2 saw offset still gives the best mono behavior.
3. Add additional morph targets later only if the current sine/saw contrast feels too narrow.

## Hardware Test Checklist
- Confirm X reaches exact pure-sine and pure-saw endpoints.
- Confirm Y smoothly moves from stable to nervous behavior in Normal mode.
- Confirm Alt holds waveform choices longer at low Y and rerolls quickly at high Y.
