# Karplus-Strong / Digital Waveguide

## Verdict

`Selected`

This is the most practical resonator-style candidate in the batch.
The repo already contains the exact kind of helper and reference structure that
make a first implementation realistic on the Computer Card.

## Core Idea

Use a short delay line as a resonant loop.
Excite it with noise, a short impulse, or an external burst, then feed the
delayed signal back through simple filtering and damping.

A compact first-order description is:

- choose delay length `L` from pitch
- read the delayed sample
- filter it for brightness / loss
- feed a scaled version back into the delay line
- add an excitation burst when triggered

This produces plucked-string, struck, and hybrid resonator sounds with very
little math per sample.

## Why It Fits Knots

- `reference/utility_pair/delayline.h` is already available.
- `reference/utility_pair/main.cpp` contains a Karplus-Strong example.
- The algorithm is proven on small embedded targets.
- It adds a musical category Knots does not currently have: pitched resonator /
  pluck / drum behavior.

## Likely Control Map

- `Main`
  - pitch via delay length
- `X`
  - decay / damping / feedback loss
- `Y`
  - excitation brightness or pick position
- `Alt`
  - string-like vs drum/tube-like feedback/filter variant
- `Out1`
  - main resonator output
- `Out2`
  - complementary filtered tap, body tap, or excitation-heavy variant

This uses the panel well:

- `X` changes how long the body rings
- `Y` changes how the hit or pluck feels

## Reference Routing

Checked first:

- `reference/utility_pair/delayline.h`
- the `karplusstrong` example in `reference/utility_pair/main.cpp`
- the shared `ExpVoct` and one-pole filter helpers listed in `AGENT_REFERENCE.md`

Primary external papers:

- Digital Synthesis of Plucked-String and Drum Timbres
- Extensions of the Karplus-Strong Plucked-String Algorithm

## Reuse Plan

Reuse first:

- `DelayLine`
- Utility-Pair high-pass and low-pass patterns
- `ExpVoct` pitch conversion
- existing Knots control-frame split for precomputing delay length and damping

Why this is a strong reuse story:

- unlike several other ideas in the batch, the repo already has both a generic
  helper and a worked musical example

## Control-Rate Vs Audio-Rate Split

### Control-rate

- compute the target delay length from pitch
- map damping and brightness
- choose the Alt-mode feedback/filter topology
- precompute fractional delay values if interpolation is used

### Audio-rate

- read the delay line
- apply damping / tone filter
- add or decay excitation
- write the feedback sample

## Hot-Path Risks

- Very short delays at high pitch need careful interpolation.
- Feedback must stay clearly below runaway.
- Excitation should not add DC.
- Memory use is higher than the phase-family engines, but still reasonable for
  one or two small delay lines.

This is a likely place to use `__not_in_flash_func()` if the final inner loop
adds interpolation, a filter, and a body tap.

## First Prototype Shape

Normal mode:

- plucked string or lightly damped resonator

Alt mode:

- drum / bottle / harsher sign-flip or alternate filter variant

Trigger ideas:

- self-retrigger on wrap for drone-like behavior
- `Pulse In 1` or `Pulse In 2` can be explored later, but v1 does not need to
  depend on external triggers to be playable

## What To Test

- pitch tracking over the useful range
- decay length and damping feel across X
- brightness / excitation feel across Y
- high-pitch stability when the delay becomes very short
- differences between Normal and Alt at the same pitch

## Sources

- https://compmus.com/icm-online/readings/icm-week-11.pdf
- https://musicweb.ucsd.edu/~trsmyth/papers/KSExtensions.pdf
- https://users.soe.ucsc.edu/~karplus/papers/digitar.pdf
