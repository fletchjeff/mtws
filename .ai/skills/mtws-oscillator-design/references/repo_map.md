# Repo Map for Oscillator Design

Use this file to choose the smallest set of repo references that answer the current design question.

## Policy and Constraints
- [../../../../AGENTS.md](../../../../AGENTS.md): workflow, test loop, and realtime rules
- [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md): helper reuse order, Utility-Pair source map, and bootstrap patterns

## Platform and Reference Routing
- [../../../../ComputerCard.h](../../../../ComputerCard.h): Workshop System platform API, jack semantics, and calibrated helper behavior
- [../../../../reference/workshop_computer_examples/](../../../../reference/workshop_computer_examples/): USB, MIDI, second-core, calibration, and browser-tooling examples
- [../../../../reference/10_twists/](../../../../reference/10_twists/): larger oscillator architecture, interpolation, resources/settings, and USB worker patterns

## Good Existing Engine Anchors
- [../../../../knots/solo_engines/bender/](../../../../knots/solo_engines/bender/): simplest solo pattern for a new oscillator scaffold
- [../../../../knots/solo_engines/sawsome/](../../../../knots/solo_engines/sawsome/): use when voice spread, detune maps, or multi-voice stereo logic matter
- [../../../../knots/src/engines/bender_engine.cpp](../../../../knots/src/engines/bender_engine.cpp): simplest integrated-slot control/audio split
- [../../../../knots/src/engines/din_sum_engine.cpp](../../../../knots/src/engines/din_sum_engine.cpp): heavier control-rate caching and hot-path optimization examples

## Docs to Read Selectively
- [../../../../docs/engines/](../../../../docs/engines/): current engine goals, control maps, and test checklists
- [../../../../docs/MTWS_USER_GUIDE.md](../../../../docs/MTWS_USER_GUIDE.md): integrated build behavior that overrides standalone docs when they differ
- [../../../../knots/src/engines/bender_engine.cpp](../../../../knots/src/engines/bender_engine.cpp): simple integrated optimization patterns
- [../../../../knots/src/engines/din_sum_engine.cpp](../../../../knots/src/engines/din_sum_engine.cpp): cached-frame and hot-path optimization patterns

## Utility-Pair Files to Check First
- `../../../../AGENT_REFERENCE.md` already points to the most useful siblings in `reference/utility_pair/`
- Start with `reference/utility_pair/main.cpp`, `delayline.h`, `bernoulligate.h`, `divider.h`, `Saw.h`, and `t185_state.h`

## When to Read Other Reference Buckets
- If the task changes jack behavior, calibration, or MIDI/CV helper usage, read `../../../../ComputerCard.h` first.
- If the task touches USB, MIDI transport, second-core scheduling, calibration, or browser tooling, inspect the smallest relevant example under `../../../../reference/workshop_computer_examples/`.
- If the task needs a larger oscillator structure, smoothing/interpolation, or service-loop patterns outside the audio hot path, inspect the smallest relevant file under `../../../../reference/10_twists/`.
