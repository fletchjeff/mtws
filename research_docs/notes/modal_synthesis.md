# Modal Synthesis

## Verdict

`Selected`, but only in a reduced form.

This stays in the six because it can add bells, bars, plates, and unreal metal
tones that are not already present in Knots.
It only stays practical if the implementation is a compact fixed mode bank with
precomputed ratios and decays.

## Core Idea

Represent a resonant object as a sum of damped modes.
Each mode has:

- a frequency ratio
- a gain
- a decay constant

At audio rate the engine becomes:

- update each mode phase
- apply its decay
- sum the damped sinusoids

The cited modal paper focuses on more general preprocessing and object-space to
mode-space transforms.
That is useful conceptually, but it is not the right v1 implementation target
for Knots.

## Why It Fits Knots

- The sonic territory is valuable.
- The underlying audio-rate work resembles a small additive bank, which is
  familiar from `Cumulus`.
- A fixed bank can keep the math deterministic and lightweight enough for the
  MCU.

## Why It Needs A Scope Limit

Do not try to ship:

- runtime eigendecomposition
- arbitrary object import
- large mode counts
- general force-to-mode matrix pipelines

Do ship:

- a few curated modal families
- a small number of modes, likely `6..8`
- precomputed tables for ratio, gain, and default damping

## Likely Control Map

- `Main`
  - base pitch or object tuning
- `X`
  - material / brightness / mode emphasis
- `Y`
  - decay or strike position / excitation distribution
- `Alt`
  - bank switch such as bar vs bell or plate vs tube
- `Out1`
  - full modal sum
- `Out2`
  - brighter or sparser companion mix

## Reference Routing

Checked first:

- `Cumulus` for multi-voice oscillator organization
- `SineLUT` usage
- the modal synthesis paper for the conceptual reduction from object space to a
  runtime bank of damped sinusoids

Primary external paper:

- Interactive Modal Sound Synthesis Using Generalized Proportional Damping

## Reuse Plan

Reuse first:

- multi-voice additive structure ideas from `Cumulus`
- control-rate precomputation of phase increments and gain scalars
- `SineLUT` for each mode

Not reusing a direct Utility-Pair helper because:

- the repo has additive and delay patterns, but no ready-made modal bank

## Control-Rate Vs Audio-Rate Split

### Control-rate

- choose the modal family
- precompute mode frequencies from the base pitch
- precompute excitation weights from strike position or brightness
- precompute per-mode decay coefficients

### Audio-rate

- advance the active mode phases
- update decays
- sum the bank

## Hot-Path Risks

- Mode count is the main CPU lever.
- Too many high modes can waste CPU above Nyquist.
- Poor normalization can make banks jump in level.
- If the mode bank is too small, the result risks overlapping too much with the
  existing additive engine.

The strong rule for v1 is to keep the mode count low and the banks curated.

## First Prototype Shape

Normal mode:

- denser bell/bar family

Alt mode:

- alternate family with a different decay personality

Stereo idea:

- full mix on Out1
- odd/even or bright/dark split on Out2

## What To Test

- does the engine feel different enough from `Cumulus`
- do the curated banks each have a clear identity
- does Y create a useful strike-position or decay gesture
- does the mode count stay inside the callback budget at high pitch
- are the two outputs musically paired

## Sources

- https://www.cs.unc.edu/~austonst/files/gpdsynth.pdf
