# Feedback AM / Nonlinear Resonator Variants

## Verdict

`Selected`

This is one of the best low-state additions in the whole list.
It can produce a rich, pulse-like, nonlinear sound while staying light enough
to respect the Computer Card budget if the implementation is carefully bounded.

## Core Idea

The basic feedback AM oscillator can be expressed as a sinusoid multiplied by a
feedback term derived from the previous output:

- `y[n] = carrier[n] * (1 + a * y[n - 1])`

The external papers interpret the structure as a coefficient-modulated one-pole
IIR filter.
The important practical consequences are:

- it can create rich spectra from very little state
- it needs explicit attention to scaling, stability, and DC behavior

## Why It Fits Knots

- minimal memory footprint
- tiny persistent state
- different sonic territory from the existing six engines
- easy to express with the current control-frame model

It is a good counterpart to the heavier resonator candidates because it gives a
big timbral return without asking for a big buffer or mode bank.

## Likely Control Map

- `Main`
  - pitch
- `X`
  - feedback amount
- `Y`
  - harmonic theme / brightness / variant selection
- `Alt`
  - first-order vs second-order or alternate feedback topology
- `Out1`
  - smoother, safer core voice
- `Out2`
  - brighter or more aggressive companion voice

The exact meaning of `Y` should be chosen to preserve a stable sweet spot rather
than expose too many abstract filter-theory parameters.

## Reference Routing

Checked first:

- existing Knots sine/LUT oscillator patterns
- Utility-Pair high-pass helper references in `AGENT_REFERENCE.md`
- the Maynooth feedback AM papers

Primary external papers:

- Feedback Amplitude Modulation Synthesis
- Aspects of Second-Order Feedback AM Synthesis

## Reuse Plan

Reuse first:

- `SineLUT`
- simple phase accumulator structure
- high-pass / DC-block patterns from Utility-Pair references
- existing clamp and level-management ideas from `Bender`

Why not reuse a full helper:

- the repo does not yet include a self-contained feedback AM oscillator block

## Control-Rate Vs Audio-Rate Split

### Control-rate

- clamp the feedback amount into a safe range
- map `Y` to a small family of musical variants
- precompute gain compensation if needed
- choose the Alt-mode topology

### Audio-rate

- advance the carrier phase
- read the carrier waveform
- update one or two feedback states
- DC-block and clamp the output

## Hot-Path Risks

- DC build-up
- abrupt output-level changes
- runaway at extreme feedback
- extra aliasing as the waveform narrows and sharpens

This is a candidate where the safety design is part of the timbre design.
It should be impossible to enter a numerically unstable region from the front
panel.

## First Prototype Shape

Normal mode:

- first-order feedback AM with safe compensation and DC blocking

Alt mode:

- second-order or alternate feedback arrangement, but only if it remains stable

Out split:

- Out1 should bias toward the safer and more continuously playable voice
- Out2 can carry the more extreme variant

## What To Test

- stable output across all X positions
- clear timbral change from Y without level collapse
- DC-block effectiveness
- high-note behavior at aggressive feedback settings
- Alt mode difference without obvious instability

## Sources

- https://mural.maynoothuniversity.ie/4193/1/VL_feedback.pdf
- https://mural.maynoothuniversity.ie/id/eprint/4118/1/aspects-of-second-order-feedback-am-synthesis.pdf
