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
4. Move `Z` to `Down` to advance to the next slot.
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

- The current code powers up on slot `6`, `din_sum`.
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
- `Z Down`: advance to the next slot on entry.
- While `Z` is held `Down`, the `X` knob configures `Pulse2 output` clock behavior.

Important current-build detail:

- Entering `Z Down` always advances the slot once before you use it as clock-setup mode.

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
2. The slot advances once when you enter `Down`.
3. Keep `Z` held `Down`.
4. Turn `X`.
5. If external MIDI clock is active, `X` selects the output division.
6. If external MIDI clock is inactive, `X` sets the internal BPM.

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

- Waveshaping oscillator with fold mode and ramp-morph Alt mode.
- `X`: fold amount in Normal, morph amount in Alt.
- `Y`: bias in Normal, Out2 phase shift in Alt.
- More detail: [engines/bender.md](engines/bender.md)

### 3. floatable

- Stereo wavetable oscillator with independent Out1 and Out2 table positions.
- `X`: Out1 wavetable position.
- `Y`: Out2 wavetable position.
- `Alt`: switches to the alternate bank pair.
- More detail: [engines/floatable.md](engines/floatable.md)

### 4. cumulus

- Additive oscillator with odd/even output split.
- `X`: centroid position.
- `Y`: spread in Normal, warp in Alt.
- `Alt`: ratio-warp mode.
- More detail: [engines/cumulus.md](engines/cumulus.md)

### 5. losenge

- Dual-formant vowel-style synth slot.
- `X`: Out1 formant morph.
- `Y`: Out2 formant morph.
- `Alt`: alternate carrier and vowel set.
- More detail: [engines/losenge.md](engines/losenge.md)

### 6. din_sum

- Noise-based split-filter slot.
- `X`: damping / Q behavior.
- `Y`: filter distance from center.
- `Alt`: alternate noise source and filter topology.
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

- [MTWS_TODO.md](MTWS_TODO.md)
- [MTWS_6OSC_MASTER_PLAN.md](MTWS_6OSC_MASTER_PLAN.md)
- [engines/sawsome.md](engines/sawsome.md)
- [engines/bender.md](engines/bender.md)
- [engines/floatable.md](engines/floatable.md)
- [engines/cumulus.md](engines/cumulus.md)
- [engines/losenge.md](engines/losenge.md)
- [engines/din_sum.md](engines/din_sum.md)
