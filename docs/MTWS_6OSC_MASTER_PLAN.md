# MTWS 6-Oscillator Master Plan

## Goal
Unify the project into a single modular `mtws` firmware with 6 oscillator slots, global USB MIDI device input, multicore control offload, and shared control/audio behavior.

## Slot Map
1. Virtual Analogue (placeholder)
2. Wavetable (rebuilt)
3. Additive (ported from optimized implementation)
4. Wave Shaping (placeholder)
5. Formant/Voice (placeholder)
6. Filtered Noise (placeholder)

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
- [x] Add new `mtws` CMake target while retaining `additive` and `wavetable`
- [x] Create modular `prex/mtws` source layout
- [x] Add shared control/frame types (`UISnapshot`, `MidiState`, `GlobalControlFrame`, `EngineControlFrame`)
- [x] Add engine interface + slot registry
- [x] Implement core1 MIDI worker + control frame publishing
- [x] Implement core0 render host + slot switch edge detect + crossfade
- [x] Implement selected-slot LED behavior + MIDI note flash pulse
- [x] Implement CVOut1 MIDI note tracking (hold last)
- [x] Implement placeholder oscillator modules with audible alt variants
- [x] Port additive engine into slot 3 with control/render split
- [x] Rebuild wavetable engine in slot 2 using fixed-point render path
- [x] Apply CV2 VCA post-engine per output bus
- [ ] Build/flash validation on hardware for all acceptance tests

## Acceptance Test Matrix
- [ ] `mtws`, `additive`, and `wavetable` build cleanly.
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
- [ ] Additive slot behavior matches existing optimized baseline.
- [ ] Wavetable slot functions under shared global control path.
- [ ] Stable under simultaneous MIDI + switching + control movement.
