# Pulsar Synthesis

## Verdict

`Selected`, but only as a tightly scoped oscillator.

Pulsar synthesis stays because it could give Knots a strange, formant-rich,
rhythmic-to-pitched voice that none of the current engines cover.
It only stays practical if the design is periodic and bounded.

## Core Idea

Generate a repeating train of very short pulsarets.
Each pulsaret is a tiny waveform multiplied by a window.

A practical oscillator framing is:

- one master phase controls the overall period
- a secondary local phase controls the current pulsaret shape
- density and width determine how many pulses live inside each cycle and how
  narrow they are

This lets the engine move between clearly pitched buzzing tones and more
formant-like rhythmic textures without needing a free-form grain scheduler.

## Why It Fits Knots

- It can be built from oscillators and envelopes instead of buffers.
- It adds a distinctly digital, animated voice to the lineup.
- It is a better fit if treated as a deterministic oscillator than as a generic
  granular environment.

## Scope Guard

Do not try to build:

- a cloud of asynchronously scheduled grains
- arbitrary sample-based grains
- broad random timing systems in v1

Do build:

- a periodic pulsaret oscillator
- a small set of windows or a single cheap window function
- a musically bounded density range

## Likely Control Map

- `Main`
  - base repetition pitch
- `X`
  - pulsaret width / window width
- `Y`
  - density / formant spacing / number of pulses per cycle
- `Alt`
  - harmonic locked vs more asymmetric / nervous pulse arrangement
- `Out1`
  - cleaner periodic pulsar voice
- `Out2`
  - contrasting or offset pulsar arrangement

## Reference Routing

Checked first:

- existing phase-based Knots oscillators
- LUT and control-frame patterns
- the pulsar and granular overview papers for the core conceptual boundaries

Primary external papers:

- Sound Composition with Pulsars
- The path to Half Life
- The Evolution of Granular Synthesis

## Reuse Plan

Reuse first:

- phase accumulator structure
- `SineLUT` or another small waveform source for the pulsaret
- control-rate mapping and stereo-variant patterns from current engines

Why reuse is weaker than the phase or waveguide families:

- the repo has no direct pulsar helper, so most of the engine logic will still
  be new

## Control-Rate Vs Audio-Rate Split

### Control-rate

- map width and density
- choose the Alt-mode pulse arrangement
- precompute safe ranges so very narrow pulses do not immediately become
  unusable

### Audio-rate

- advance master phase
- derive local pulsaret phase
- evaluate a cheap window
- evaluate the pulsaret waveform
- apply output normalization

## Hot-Path Risks

- Very narrow pulses invite aliasing fast.
- Loudness can jump a lot as width shrinks or density rises.
- If the window function is too fancy, the engine loses its practical value.

The safest v1 path is a simple periodic design with a small harmonic-density
range and very clear bounds on width.

## First Prototype Shape

Normal mode:

- stable periodic pulsar voice

Alt mode:

- more aggressive, asymmetric, or denser variant

Prototype constraint:

- if the sound starts behaving like uncontrolled broadband buzz for most of the
  knob travel, the design is too wide and needs to be narrowed

## What To Test

- pitched behavior over the useful range
- smooth shift between broader and narrower pulses
- density control should feel musical, not just noisy
- normalization should prevent huge level jumps
- Out1 and Out2 should widen the sound without becoming random

## Sources

- https://static1.squarespace.com/static/6544ee636e12b3162cadf673/t/6548d1728d5942241587863f/1699271043689/SoundCompwithPulsars.pdf
- https://static1.squarespace.com/static/5ad03308fcf7fd547b82eaf7/t/5b75a17b88251b44e5693441/1534435708241/04%2BPath%2Bto%2BHalf%2BLife.pdf
- https://static1.squarespace.com/static/5ad03308fcf7fd547b82eaf7/t/5b75a255352f53388d8ef793/1534435933359/EvolutionGranSynth_9Jun06.pdf
