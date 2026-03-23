# Chaotic / Nonlinear Dynamical Oscillators

## Verdict

`Abandoned for this batch`

This is the easiest candidate to admire and the hardest to justify inside the
current Knots schedule.

## Core Idea

Use a nonlinear dynamical system or chaotic process as the sound source, often
followed by resonators or filter banks.
The attraction is obvious:

- unstable, quasi-periodic motion
- sensitive responses to control
- unexpected edge behavior

The trouble is that those same strengths are also implementation hazards.

## Why It Fell Out

- Pitch identity is often weaker and harder to preserve.
- State history matters a lot, which makes it trickier to make repeatable and
  playable.
- Instability and DC risk are much higher than in the other selected methods.
- The supporting structure often wants resonators, bandpass banks, or larger
  validation effort before the sound becomes musically reliable.

For this batch, it looks more like a long-form research project than a practical
next Knots engine.

## Reuse Outlook

Very little direct reuse.

Possible future path:

- reconsider as an effect, noise source, or hybrid resonator exciter
- not as one of the main six new oscillator additions

## What To Test If It Ever Returns

- pitch stability across the full panel range
- startup state and reset behavior
- DC bias and limiter behavior
- how quickly the engine crosses from exciting to unusable

## Sources

- https://2019.xcoax.org/pdf/xCoAx2019-Mudd.pdf
