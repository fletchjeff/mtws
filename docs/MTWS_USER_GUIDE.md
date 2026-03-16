# MTWS User Guide

This guide covers the integrated `mtws` firmware in `prex/mtws`.

- It describes the shared panel, CV, pulse, LED, and MIDI behavior that all six slots use in the integrated build.
- For engine-specific sound design details, see the per-engine docs in `docs/engines/`.
- If a standalone engine doc differs from this guide, this guide takes precedence for the integrated `mtws` target.

## What MTWS Is

`mtws` is the combined six-slot ComputerCard firmware.

- Audio runs at `48kHz`.
- Shared control and MIDI work update at about `1kHz`.
- Every slot uses the same global panel and I/O rules.
- The integrated build adds global USB MIDI behavior, CV/pulse outputs, slot selection, and `Pulse2` clock output on top of the engine DSP.

## Quick Start

1. Patch `Audio1 out` and `Audio2 out` to your mixer or monitor path.
2. Use `Main` to tune the current slot.
3. Use `Z Up` for Alt and `Z Middle` for Normal.
4. Tap `Z` to `Down` and let it return to `Middle` to advance to the next slot.
5. Patch `Audio1 input` or `Audio2 input` if you want external modulation of `X` or `Y`.
6. Patch `CV2 input` if you want a post-engine VCA.
7. Connect USB MIDI if you want note, gate, CV, and clock behavior.
8. Patch `Pulse2 output` if you want clock out.

## Slot Order

The integrated build currently uses this slot order:

1. `sawsome`
2. `bender`
3. `floatable`
4. `cumulus`
5. `losenge`
6. `din_sum`

Current behavior notes:

- The current code powers up on slot `1`, `sawsome`.
- Slot changes are immediate hard cuts in the current build.
- You can advance slots from the `Z` switch or from `Pulse2 input`.

## Global Panel Behavior

### Main

- `Main` sets the base pitch for the current slot.
- Internally it maps across a wide range, approximately `10Hz..10kHz`, before MIDI transposition is applied.

### X and Y

- With no matching audio input patched, `X` and `Y` directly control the engine macros.
- If `Audio1 input` is patched, `Audio1` becomes the source for macro `X` and the `X` knob becomes an attenuator.
- If `Audio2 input` is patched, `Audio2` becomes the source for macro `Y` and the `Y` knob becomes an attenuator.

### Z Switch

- `Z Up`: latch Alt mode.
- `Z Middle`: latch Normal mode.
- `Z Down`: arms one slot advance for the return to `Middle`.
- While `Z` is held `Down`, the `X` knob configures `Pulse2 output` clock behavior.
- Moving `X` while `Z` is held `Down` cancels that pending slot advance and turns the gesture into clock setup.
- Pressing `Z` alone does not change the stored clock setting. The clock setting changes only after a deliberate `X` move while `Z` is held.

Important current-build detail:

- Slot advance now happens on the debounced `Down -> Middle` return, not on entry into `Down`.

## Inputs

### CV1 Input

- `CV1 input` is added to `Main` before MIDI note transpose is applied.
- Use it as pitch CV into the currently selected slot.

### CV2 Input

- `CV2 input` is a post-engine global VCA control.
- With `CV2 input` unplugged, output level is unity.
- `0V` and below closes the VCA.
- `+5V` and above reaches full level.
- The gain is lightly smoothed to reduce clicks when the control changes.

### Audio1 and Audio2 Inputs

When `Audio1 input` or `Audio2 input` is patched:

- The incoming signal is treated as macro modulation instead of plain audio pass-through.
- The matching knob becomes a depth control.
- Inputs are clamped to approximately `-5V..+5V` before mapping.
- At full knob, `-5V` maps to macro minimum, `0V` to about midpoint, and `+5V` to macro maximum.
- At zero knob, the macro stays at `0`.

When those inputs are unplugged:

- `X` and `Y` act as direct manual controls again.

### Pulse1 Input

- `Pulse1 input` overrides the panel Alt latch when a jack is detected.
- `Pulse1 input` low = Normal.
- `Pulse1 input` high = Alt.
- While `Pulse1 input` is patched, `Z Up` and `Z Middle` still update the latched panel state, but the live mode follows the pulse input instead.

### Pulse2 Input

- `Pulse2 input` advances to the next slot on each rising edge.
- This only happens when the `Pulse2 input` jack is detected.
- While `Z` is held `Down`, `Pulse2 input` slot advance is suspended until the gesture completes.

## Outputs

### Audio Outputs

- `Audio1 out` carries the current slot's `Out1` signal.
- `Audio2 out` carries the current slot's `Out2` signal.

### CV Outputs

- `CV1 out` follows the last received MIDI note.
- `CV1 out` holds that note after note-off.
- `CV2 out` follows MIDI `CC74`.
- `CV2 out` spans the raw board CV range, approximately `-6V..+6V`.

### Pulse Outputs

- `Pulse1 out` is a MIDI gate output.
- `Pulse1 out` is high while a MIDI note is active.
- `Pulse2 out` is the clock output described below.

## MIDI Behavior

The integrated `mtws` build currently acts as a USB MIDI input target for these features:

- Note on/off
- Note pitch transpose
- `CC74` to `CV2 out`
- MIDI clock to `Pulse2 out`

Current MIDI rules:

- Note and CC handling are channel-1 only.
- MIDI clock is handled as system real-time clock.
- MIDI note `60` is the zero-transpose reference for the panel pitch.
- `Main + CV1 input` define the local base pitch, and MIDI notes transpose from there.
- The most recent MIDI note is held through note-off, so pitch and `CV1 out` do not snap back when the gate drops.
- `Pulse1 out` goes high while a note is active and low on note-off.
- MIDI CC-to-`X` and CC-to-`Y` mapping is intentionally disabled in the current build.

## Pulse2 Clock Output

`Pulse2 out` is the clock output for the integrated `mtws` build.

- With external MIDI clock present, it outputs divided pulses derived from incoming `24 PPQN` MIDI clock ticks.
- When MIDI clock is absent, it falls back to an internal clock.
- If external MIDI clock stops for about `0.5s`, the output switches to the internal clock automatically.
- If external MIDI clock starts again, the output returns to divided external clock automatically.
- Output pulse width is about `5ms`.

Important scope note:

- This clock currently drives `Pulse2 out` only.
- It does not tempo-sync the audio engines internally.

### How To Set Pulse2 Clock

1. Move `Z` to `Down`.
2. Keep `Z` held `Down`.
3. Turn `X` if you want to edit the clock setting.
4. If external MIDI clock is active, `X` selects the output division.
5. If external MIDI clock is inactive, `X` sets the internal BPM.
6. Return `Z` to `Middle` when you are done.

Clock-setup gesture note:

- If `X` stays still while `Z` is held, the release back to `Middle` advances one slot.
- If `X` moves while `Z` is held, that release does not advance the slot.
- The previous BPM or clock division stays in effect until `X` has moved enough to claim the clock-edit gesture.

### External MIDI Clock Divisions

The `X` knob is split into eight equal regions while `Z` is held `Down`.

From full counter-clockwise to full clockwise:

| X region | Incoming ticks per pulse | Approximate musical rate |
| --- | ---: | --- |
| 1 | `24` | quarter note |
| 2 | `12` | eighth note |
| 3 | `8` | eighth-note triplet |
| 4 | `6` | sixteenth note |
| 5 | `4` | sixteenth-note triplet |
| 6 | `3` | thirty-second note |
| 7 | `2` | thirty-second-note triplet |
| 8 | `1` | raw `24 PPQN` clock |

### Internal Clock Range

- Internal BPM spans `20..999`.
- The BPM mapping is linear across the `X` knob while `Z` is held `Down`.

## LEDs

- There are six slot LEDs.
- The selected slot LED is lit during normal operation.
- On each MIDI note-on, the selected slot LED briefly blanks for about `20ms`.

## Engine Summaries

The integrated build uses these six engines:

### 1. sawsome

- Detuned stereo supersaw / supertriangle slot.
- `X`: stereo width.
- `Y`: detune depth.
- `Alt`: triangle variant instead of saw.
- The integrated `mtws` build currently renders a 7-voice spread.
- More detail: [engines/sawsome.md](engines/sawsome.md)

### 2. bender

- Waveshaping oscillator that compares fold and crush routings.
- `X`: fold drive and fold wet mix.
- `Y`: crush amount.
- `Alt`: reorders the fold/crush stages.
- More detail: [engines/bender.md](engines/bender.md)

### 3. floatable

- Stereo wavetable oscillator with independent Out1 and Out2 morph positions.
- `X`: Out1 wavetable position.
- `Y`: Out2 wavetable position.
- `Alt`: switches to the alternate source bank.
- More detail: [engines/floatable.md](engines/floatable.md)

### 4. cumulus

- Additive oscillator with odd/even output split.
- `X`: centroid position.
- `Y`: spread in Normal, warp in Alt.
- `Alt`: ratio-warp mode.
- More detail: [engines/cumulus.md](engines/cumulus.md)

### 5. losenge

- Formant vowel oscillator with mirrored stereo vowel motion.
- `X`: vowel path position.
- `Y`: shared tract/formant shift.
- `Alt`: brighter upper-formant set.
- More detail: [engines/losenge.md](engines/losenge.md)

### 6. din_sum

- Stochastic sine-to-saw morph slot.
- `X`: morph position / saw probability.
- `Y`: coherence / cycle hold behavior.
- `Alt`: cycle-locked probabilistic mode.
- More detail: [engines/din_sum.md](engines/din_sum.md)

## Current Limitations And Caveats

- Slot changes are hard cuts in the current build.
- `Z Down` is shared between slot advance and `Pulse2` clock setup.
- `Pulse1 input` overrides the panel Alt latch whenever it is patched.
- MIDI note and CC handling are channel-1 only.
- MIDI macro control of `X` and `Y` is intentionally disabled.
- `Pulse2 out` clocking does not currently sync the engines internally.
- `CV2 input` is a unipolar VCA input, not a bipolar modulation input.

## Related Docs

- [engines/](engines/)
- [HOST_PREVIEW.md](HOST_PREVIEW.md)
- [engines/sawsome.md](engines/sawsome.md)
- [engines/bender.md](engines/bender.md)
- [engines/floatable.md](engines/floatable.md)
- [engines/cumulus.md](engines/cumulus.md)
- [engines/losenge.md](engines/losenge.md)
- [engines/din_sum.md](engines/din_sum.md)
