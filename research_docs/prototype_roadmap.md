# Prototype Roadmap

## Phase 1: Research And Triage

Deliverables:

- one note per candidate technique
- this feasibility matrix
- local source PDFs in `pdfs/`
- final six-engine selection

Exit criteria:

- every candidate has a concrete MCU-fit reading
- the selected 6 each have an initial control map
- the rejected 2 each have a clear reason for staying out of this batch

## Phase 2: Solo Prototypes

Build order:

1. `Phase distortion / phaseshaping`
2. `WPPM`
3. `Karplus-Strong / digital waveguide`
4. `Feedback AM`
5. `Modal synthesis`
6. `Pulsar synthesis`

Why this order:

- Start with the two phase-family oscillators while the control and aliasing
  lessons are still fresh.
- Move next to the safest resonator family with the strongest repo reuse story.
- Keep the more open-ended banked and pulse-train engines for later, once the
  first four have already expanded Knots in obvious ways.

Each solo prototype should prove:

- it builds cleanly as a standalone target
- it survives high pitch without obvious callback overruns
- `Main`, `X`, `Y`, and `Alt` all land in musically useful ranges
- it feels distinct from the existing six engines

## Phase 3: Integrated Host Expansion To 12

Do not start this phase until at least the first solo prototypes validate the
selected six.

Expected implementation work:

- change `kNumOscillatorSlots` from `6` to `12`
- expand `EngineRegistry`
- extend `EngineControlFrame`
- keep the current single-step slot cycle
- add LED coding for slots `7..12`
- update host docs after the new engines are stable

Recommended integration order:

1. Add the two phase-family engines first.
2. Add Karplus-Strong and Feedback AM second.
3. Add Modal and Pulsar last.

That order keeps the bigger stateful engines away from the first host-expansion
pass.

## Phase 4: Polish And Documentation

Only after integrated behavior is stable:

- update `docs/USER_GUIDE.md`
- add one engine doc per new oscillator
- document the 12-step slot cycle and upper-slot LED code
- record build sizes and any engines that need profiling notes
