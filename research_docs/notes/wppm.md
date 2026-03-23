# Wave Pulse Phase Modulation (WPPM)

## Verdict

`Selected`

WPPM is one of the most distinctive ideas in the batch.
It stays because it can reuse the same phase-centric substrate as a simpler
phaseshaping engine while delivering a more signature sound.

## Core Idea

WPPM segments one phase ramp into two regions and uses each region to drive its
own windowed modulation pulse.
Those pulses are then folded back into the carrier phase, combining ideas from
Phase Distortion and Phase Modulation.

A practical Knots reading is:

- linear phase ramp `phi`
- split point `d`
- region A generates one windowed modulation pulse
- region B generates another windowed modulation pulse
- both pulses reshape the phase of a sinusoidal carrier

The DAFx 2025 paper emphasizes that this yields a rich sound with a fairly small
parameter set, which is exactly the right direction for the panel.

## Why It Fits Knots

- It is still mainly phase math plus LUT reads.
- It has almost no memory footprint compared with scanned or chaotic methods.
- It sounds unlike the current six engines if we keep the pulse pair expressive.
- It can share control ideas and implementation scaffolding with the simpler
  phase-distortion prototype.

## Likely Control Map

- `Main`
  - oscillator pitch
- `X`
  - pulse split / duration balance between the two phase regions
- `Y`
  - modulation index or window strength
- `Alt`
  - switch between a stable harmonic ratio set and a freer asymmetric pulse
    variant
- `Out1`
  - primary WPPM voice
- `Out2`
  - companion voice with mirrored split or alternate ratio pairing

This keeps the engine playable with only two knobs:

- `X` changes where the two phase pulses live
- `Y` changes how strongly they bend the carrier

## Reference Routing

Checked first:

- the WPPM paper itself
- existing Knots phase-accumulator engines
- `SineLUT` and control-frame patterns

Primary external paper:

- Wave Pulse Phase Modulation: Hybridising Phase Modulation and Phase
  Distortion

## Reuse Plan

Reuse first:

- the same phase accumulator + LUT structure used by phase distortion
- `ControlTick()` precomputation for split and ratio mapping
- any reciprocal or clamp helpers created for the simpler phaseshaping engine

Not reusing a Utility-Pair helper directly because:

- there is no existing helper there that already implements the two-region
  windowed phase-pulse behavior

## Control-Rate Vs Audio-Rate Split

### Control-rate

- map `X` to the split point
- map `Y` to modulation index and/or window depth
- choose safe ratio sets for `R1` and `R2`
- precompute region scaling terms to avoid audio-rate divides

### Audio-rate

- advance phase
- map phase into region A or B
- evaluate one or two windowed sine modulators with the LUT
- sum the phase offset into the carrier lookup

## Hot-Path Risks

- Two modulation reads plus one carrier read cost more than plain PD.
- Abrupt region boundaries can get aliased if the windowing is not smooth.
- Free non-integer ratios are seductive, but v1 should bias toward bounded
  harmonic or musically adjacent ratios.
- Output level can swing a lot as both pulse durations and index rise.

The paper's windowing idea is the key practical guardrail here.

## First Prototype Shape

Normal mode:

- harmonically constrained WPPM with a small ratio table

Alt mode:

- freer or asymmetrical pulse pairing with a brighter, more vocal result

Prototype rule:

- do not start with the largest parameter space
- start with one split parameter, one depth parameter, and a tiny ratio family

## What To Test

- X should move the internal pulse timing cleanly across the cycle
- Y should brighten and densify the spectrum without instant collapse
- the engine should retain a stable pitch impression across the useful range
- high notes should not explode in aliasing when Alt is active
- Out2 should provide a related but clearly different companion voice

## Sources

- https://dafx.de/paper-archive/2025/DAFx25_paper_48.pdf
