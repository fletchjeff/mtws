# Next Batch Research Pack

This folder is intentionally local-only for now.
It exists to help pick six new oscillator approaches for a future 12-oscillator
Knots firmware without adding intermediate research material to the repo.

## Goals

1. Gather primary papers and implementation references for all 8 candidate
   techniques from `next_batch.md`.
2. Rank each idea against the actual Computer Card constraints.
3. Pick 6 techniques to prototype as standalone solo engines before touching
   the integrated host.
4. Leave a decision-complete roadmap for the later 12-slot host expansion.

## Baseline Constraints

- Audio callback: `48kHz`
- Approximate `ProcessSample()` budget: `20.8us`
- Control work already lives on core 1 in `knots/src/main.cpp`
- Current integrated host size baseline:
  - `text = 135272`
  - `bss = 8784`
  - measured from `arm-none-eabi-size build/knots.elf`
- Current host assumptions that matter for later integration:
  - `kNumOscillatorSlots = 6`
  - six LEDs only
  - slot advance is a single-step cycle from panel or `Pulse In 2`

## Final Triage

### Selected 6

1. `Phase distortion / phaseshaping`
2. `WPPM`
3. `Karplus-Strong / digital waveguide`
4. `Feedback AM / nonlinear resonator variants`
5. `Modal synthesis`
6. `Pulsar synthesis`

### Deferred

1. `Scanned synthesis`

### Abandoned For This Batch

1. `Chaotic / nonlinear dynamical oscillators`

## Why These 6

- The phase-based pair gives Knots a new family of bright, animated, strongly
  controllable digital timbres without large buffers.
- Karplus-Strong adds a resonator/pluck voice that fits the MCU and already has
  reference code in `reference/utility_pair/`.
- Feedback AM adds a distinctive nonlinear oscillator with very low state cost.
- Modal synthesis is worth keeping if it is reduced to a compact fixed bank
  rather than a general physical-model pipeline.
- Pulsar synthesis stays only if it is framed as a periodic pulsaret oscillator,
  not a free-form granular engine.

## Why The Other 2 Fell Out

- Scanned synthesis is musically attractive, but it needs a larger two-rate
  design, a clear scan-path UX, and more state validation than the stronger
  candidates above.
- Chaotic oscillators risk spending a lot of engineering time on instability,
  pitch ambiguity, and edge-case behavior before yielding a playable Knots
  engine.

## Files In This Folder

- `feasibility_matrix.md`
  - cross-technique scoring and final verdicts
- `prototype_roadmap.md`
  - research -> solo prototype -> 12-slot host sequence
- `pdf_manifest.md`
  - local file map for the downloaded paper pack
- `requirements.txt`
  - local Python dependencies for the notebook environment
- `start_notebook.sh`
  - launches Jupyter Notebook from the local virtual environment
- `phase_distortion_visuals.ipynb`
  - Jupyter notebook that visualizes linear phase, warped phase, sine lookup,
    output waveform, and a simple spectrum comparison
- `notes/phase_distortion.md`
- `notes/wppm.md`
- `notes/karplus_strong.md`
- `notes/feedback_am.md`
- `notes/modal_synthesis.md`
- `notes/pulsar_synthesis.md`
- `notes/scanned_synthesis.md`
- `notes/chaotic_oscillators.md`

## Prototype Order

1. Phase distortion / phaseshaping
2. WPPM
3. Karplus-Strong / digital waveguide
4. Feedback AM / nonlinear resonator variants
5. Modal synthesis
6. Pulsar synthesis

This order intentionally starts with the lowest-state, highest-confidence
engines before attempting the more open-ended resonator and pulsaret work.

## 12-Oscillator Host Direction

The current six engines stay.
The selected six become the second half of a future 12-oscillator firmware.

The host work should not start until at least the first solo prototypes prove:

- they fit the CPU budget
- they survive high-pitch edge cases
- the control maps feel musical
- they still sound different enough from the existing six engines

When the integrated host work starts, the expected direction is:

- increase `kNumOscillatorSlots` from `6` to `12`
- extend `EngineRegistry` and `EngineControlFrame`
- keep single-step slot cycling from panel and `Pulse In 2`
- use a 12-step sequence with LED coding for slots `7..12`

## Local Python Environment

The visual notebook now has a dedicated local environment:

- virtual environment: `research_docs/.venv`
- dependencies file: `research_docs/requirements.txt`
- launcher: `research_docs/start_notebook.sh`

That keeps the plotting stack out of the rest of the repo while making the
notebook repeatable.

## Primary Source Pack

### Phase Distortion / Phaseshaping

- Vector Phaseshaping Synthesis
  - https://mural.maynoothuniversity.ie/4096/1/vps_dafx11.pdf
- Phaseshaping Oscillator Algorithms for Musical Sound Synthesis
  - https://mural.maynoothuniversity.ie/id/eprint/4100/1/SMC2010.pdf
- Adaptive Phase Distortion Synthesis
  - https://mural.maynoothuniversity.ie/id/eprint/2335/1/VL_Adaptive_Phase_paper_12.pdf

### WPPM

- Wave Pulse Phase Modulation
  - https://dafx.de/paper-archive/2025/DAFx25_paper_48.pdf

### Scanned Synthesis

- Scanned Synthesis intro paper
  - https://www.mit.edu/~paris/pubs/smaragdis-icmc00.pdf

### Karplus-Strong / Digital Waveguide

- Introduction to Computer Music physical model notes
  - https://compmus.com/icm-online/readings/icm-week-11.pdf
- Extensions of the Karplus-Strong Plucked-String Algorithm
  - https://musicweb.ucsd.edu/~trsmyth/papers/KSExtensions.pdf
- Digital Synthesis of Plucked-String and Drum Timbres
  - https://users.soe.ucsc.edu/~karplus/papers/digitar.pdf

### Modal Synthesis

- Interactive Modal Sound Synthesis Using Generalized Proportional Damping
  - https://www.cs.unc.edu/~austonst/files/gpdsynth.pdf

### Pulsar Synthesis

- Sound Composition with Pulsars
  - https://static1.squarespace.com/static/6544ee636e12b3162cadf673/t/6548d1728d5942241587863f/1699271043689/SoundCompwithPulsars.pdf
- The path to Half Life
  - https://static1.squarespace.com/static/5ad03308fcf7fd547b82eaf7/t/5b75a17b88251b44e5693441/1534435708241/04%2BPath%2Bto%2BHalf%2BLife.pdf
- The Evolution of Granular Synthesis
  - https://static1.squarespace.com/static/5ad03308fcf7fd547b82eaf7/t/5b75a255352f53388d8ef793/1534435933359/EvolutionGranSynth_9Jun06.pdf

### Chaotic / Nonlinear Dynamical Oscillators

- Between Chaotic Synthesis and Physical Modelling
  - https://2019.xcoax.org/pdf/xCoAx2019-Mudd.pdf

### Feedback AM

- Feedback Amplitude Modulation Synthesis
  - https://mural.maynoothuniversity.ie/4193/1/VL_feedback.pdf
- Aspects of Second-Order Feedback AM Synthesis
  - https://mural.maynoothuniversity.ie/id/eprint/4118/1/aspects-of-second-order-feedback-am-synthesis.pdf

For the local filenames and any download-specific notes, see `pdf_manifest.md`.
