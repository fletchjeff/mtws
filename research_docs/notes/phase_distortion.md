# Phase Distortion / Phaseshaping

## Verdict

`Selected`

This is the best first prototype from the whole batch.
It gives Knots a clearly new digital timbre family while staying close to the
repo's current phase-accumulator + LUT architecture.

## Core Idea

Start with a linear phase ramp:

- `phi[n] = frac(phi[n - 1] + inc)`

Warp that phase before the carrier lookup.
A classic Casio-style piecewise phase distortion can be written in two regions:

- if `phi < d`, map the lower segment more quickly
- if `phi >= d`, map the upper segment more slowly

Then run the warped phase through a sinusoidal lookup:

- `out = -cos(2 * pi * F(phi))`

Vector phaseshaping extends this by moving one or more breakpoint vectors in
two dimensions, which makes the timbre more animated without changing the core
carrier structure.

## Why It Fits Knots

- No delay lines or large buffers.
- No dynamic allocation.
- A very small amount of persistent state: mainly phase and maybe a mirrored
  stereo phase or breakpoint state.
- Easy to split into control-rate mapping and audio-rate rendering.
- Strongly different from the current six engines, especially if we avoid
  simply recreating a plain saw/pulse family.

## Likely Control Map

- `Main`
  - oscillator pitch
- `X`
  - primary phase-warp breakpoint or morph amount
- `Y`
  - asymmetry / second vector dimension / warp sharpness
- `Alt`
  - switch between classic phase distortion and a vector phaseshaping variant
- `Out1`
  - direct warped carrier
- `Out2`
  - mirrored, offset, or inverse-warp companion output

This keeps the panel behavior legible:

- `X` moves the main timbral gesture
- `Y` changes the shape of that gesture
- `Alt` changes the algorithm family rather than merely adding drive

## Reference Routing

Checked first:

- existing phase/LUT engines in `knots/src/engines/`
- `SineLUT` usage patterns
- `bender_engine.cpp` for cheap audio-rate math and control preprocessing

Primary external papers:

- Vector Phaseshaping Synthesis
- Phaseshaping Oscillator Algorithms for Musical Sound Synthesis
- Adaptive Phase Distortion Synthesis

## Reuse Plan

Reuse first:

- `SineLUT`
- phase accumulator patterns already used in Bender, Losenge, and Din Sum
- control-rate precomputation pattern from `ControlTick()`

Likely new helper:

- a small phase-warp function or struct local to the engine

Why not reuse a full helper from `reference/utility_pair/`:

- the repo has oscillator and delay helpers there, but not a ready-made phase
  distortion block that matches the planned Knots control map

## Control-Rate Vs Audio-Rate Split

### Control-rate

- clamp and map `X` and `Y`
- precompute safe breakpoint bounds
- precompute reciprocals or slope terms so audio-rate code avoids division
- decide which warp family Alt mode is using

### Audio-rate

- advance phase
- apply piecewise warp
- read carrier LUT
- optionally produce a mirrored second output with the same precomputed terms

## Hot-Path Risks

- Avoid divisions in the warp path by precomputing slope reciprocals.
- Extreme breakpoints can create harsh corners and aliasing.
- Audio-rate two-dimensional vector motion is tempting, but it should not be the
  v1 design target if it pushes the phase math too far.

`__not_in_flash_func()` may be warranted if the final warp path grows beyond a
single piecewise mapping plus one LUT read.

## First Prototype Shape

Normal mode:

- classic warped-sine family with a strong sweep from hollow to bright

Alt mode:

- vector-style or adaptive warp using the same core carrier

Stereo idea:

- Out2 uses the same parameters but mirrored around the center point so the
  outputs stay related rather than becoming unrelated patches

## What To Test

- X sweep should move smoothly from restrained to bright without dead zones
- Y should meaningfully reshape the harmonic profile, not just change level
- high notes should stay under the callback budget
- extreme breakpoints should not collapse into silence or unusable alias spray
- Out1 and Out2 should feel paired, not redundant

## Sources

- https://mural.maynoothuniversity.ie/4096/1/vps_dafx11.pdf
- https://mural.maynoothuniversity.ie/id/eprint/4100/1/SMC2010.pdf
- https://mural.maynoothuniversity.ie/id/eprint/2335/1/VL_Adaptive_Phase_paper_12.pdf
