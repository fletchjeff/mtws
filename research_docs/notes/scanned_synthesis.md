# Scanned Synthesis

## Verdict

`Deferred`

This is musically interesting enough to keep on the long list, but not strong
enough for this six-engine batch.

## Core Idea

A slow-moving mass-spring network evolves over time.
An audio-rate scan path then reads across that evolving state to create a
pitched signal.

In other words:

- the physical network motion creates the timbral state
- the scan speed or path creates the audible oscillator

That separation is one of the technique's strengths, but it is also why the
design is not a good first-wave Knots target.

## Why It Fell Out

- It wants a more complex two-rate architecture than the other candidates.
- The scan-path UX is not obvious with only `Main`, `X`, `Y`, and `Alt`.
- It needs more persistent state than the phase-family or feedback-family
  engines.
- The musical result depends heavily on how the state is seeded, perturbed, and
  reset, which means more control-design work before the first good sound.

## Reuse Outlook

Possible reuse if revisited later:

- use core 1 or control-rate work to evolve a very small network
- use the audio loop only for the scan readout

Weak points:

- there is no direct repo helper for a mass-spring scan network
- the current engine patterns are a better fit for direct oscillators and small
  resonators

## If Revisited Later

A realistic v1 would need to be much smaller than the classic papers imply:

- a tiny fixed string or ring of masses
- a very clear scan path
- explicit reseed rules
- standalone-only first

## What To Test If It Ever Returns

- whether the scan path stays musically legible
- whether the state evolution creates repeatable sweet spots
- whether the CPU cost is justified by the resulting timbral range

## Sources

- https://www.mit.edu/~paris/pubs/smaragdis-icmc00.pdf
