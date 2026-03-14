# MTWS 6-Oscillator Master Plan

## Goal
Unify the project into a single modular `mtws` firmware with 6 oscillator slots, global USB MIDI device input, multicore control offload, and shared control/audio behavior.

## Follow-up Tracker
- Active post-plan work items are tracked in [MTWS_TODO.md](/Users/jeff/Toonbox/MTWS/mtws/docs/MTWS_TODO.md).
- Current user-facing behavior is documented in [MTWS_USER_GUIDE.md](MTWS_USER_GUIDE.md).

## Slot Map
1. sawsome
2. bender
3. floatable
4. cumulus
5. losenge
6. din_sum

## Locked Behavior
- Z down (momentary): increments slot index with wrap.
- Z up/middle: panel alt mode latch (`Up=Alt`, `Middle=Normal`).
- PulseIn1 connected: overrides panel mode (`high=Alt`, `low=Normal`).
- MIDI: USB device mode, channel 1 strict, mono last-note behavior.
- Pitch: `Main + CV1` local pitch plus MIDI semitone offset (`C4=60` reference).
- CVOut1: outputs MIDI note voltage and holds last note.
- AudioIn1/2: X/Y modulation sources when connected; knobs are attenuation depth.
- CVIn2: unipolar VCA (disconnected = unity gain).
- Output buses: separate Out1/Out2 paths, initially same signal.
- Slot transitions: short crossfade.

## Progress Checklist
- [x] Rename local folder `mtws_additive` -> `mtws`
- [x] Update root agent docs to `mtws` naming
- [x] Add new `mtws` CMake target
- [x] Create modular `prex/mtws` source layout
- [x] Add shared control/frame types (`UISnapshot`, `MidiState`, `GlobalControlFrame`, `EngineControlFrame`)
- [x] Add engine interface + slot registry
- [x] Implement core1 MIDI worker + control frame publishing
- [x] Implement core0 render host + slot switch edge detect + crossfade
- [x] Implement selected-slot LED behavior + MIDI note flash pulse
- [x] Implement CVOut1 MIDI note tracking (hold last)
- [x] Replace placeholder dispatch with concrete engine classes (`sawsome`, `bender`, `losenge`, `din_sum`)
- [x] Rename wavetable/additive integration engines to `floatable` and `cumulus`
- [x] Add standalone per-engine targets (`sawsome`, `bender`, `floatable`, `cumulus`, `losenge`, `din_sum`)
- [x] Add standalone shared scaffold (`prex/solo_common`) with core1 control/core0 audio split
- [x] Add per-engine docs in `docs/engines/`
- [x] Apply CV2 VCA post-engine per output bus
- [ ] Build/flash validation on hardware for all acceptance tests

## Acceptance Test Matrix
- [x] `mtws`, `sawsome`, `bender`, `floatable`, `cumulus`, `losenge`, and `din_sum` build cleanly.
- [ ] LEDs show selected slot correctly.
- [ ] Z-down cycles slots 1..6 with wrap.
- [ ] Slot crossfade is click-safe.
- [ ] PulseIn1 override works only when connected.
- [ ] Z up/middle alt mode works when PulseIn1 disconnected.
- [ ] MIDI notes on channel 1 affect pitch; other channels ignored.
- [ ] CVOut1 follows note pitch and holds after note-off.
- [ ] CC1/CC74 merge into X/Y macros via saturating add.
- [ ] AudioIn1/2 connected path modulates X/Y with knob attenuation.
- [ ] AudioIn1/2 disconnected path uses knob-only X/Y.
- [ ] CVIn2 controls output level when connected; unity when disconnected.
- [ ] Cumulus slot behavior matches existing optimized baseline.
- [ ] Floatable slot functions under shared global control path.
- [ ] Stable under simultaneous MIDI + switching + control movement.
