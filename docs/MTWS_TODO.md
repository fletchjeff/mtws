# MTWS TODO

This file tracks active follow-up work that is important, but not part of the
core `MTWS_6OSC_MASTER_PLAN.md` acceptance matrix. Update the status and notes
after each hardware test loop so pending work stays visible.

## Status Key
- `Open`: not started or not resolved
- `In Progress`: currently being worked on
- `Blocked`: needs another dependency or decision
- `Done`: implemented and hardware-verified

## Active Items

### 1. sawsome glitches in `mtws`
- `Status`: Done
- `Scope`: [prex/mtws/engines/sawsome_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/sawsome_engine.cpp)
- `Notes`: Removing the slot crossfade fixed the remaining glitching enough to close this item. The earlier Reese-bass alt idea is not needed because the hardware already provides two analog square waves.
- `What to test`: Compare normal vs alt in standalone `sawsome` and integrated `mtws`, then stress-test with high pitch and wide spread.
- Alternative idea for Alt is a Reese bass implementation.

### 2. Try 14 partials in cumulus
- `Status`: Done
- `Scope`: [prex/mtws/engines/cumulus_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/cumulus_engine.cpp)
- `Notes`: No longer needed after the slot-crossfade fix removed the artifacts that motivated this check.
- `What to test`: Build and hardware compare `16` vs `14` partials for CPU stability, spectral balance, and perceived loudness.

### 3. Better wavetables for floatable
- `Status`: Done
- `Scope`: [prex/floatable/floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp), [prex/mtws/wavetables/floatable_bank_1_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_1_16x256.h), [prex/mtws/wavetables/floatable_bank_2_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_2_16x256.h), [prex/mtws/wavetables/floatable_bank_3_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_3_16x256.h), [prex/mtws/wavetables/floatable_bank_4_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_4_16x256.h)
- `Notes`: Closed with the current Braids-style path. The curated browser exports are checked in as shared `4 x 16 x 256` bank headers. The first-pass `128` rendered-table experiment did not remove the remaining floatable glitches. The integrated `mtws` build now uses a Braids-style architecture: control tick publishes compact morph parameters only, and `RenderSample()` performs direct per-sample wavetable interpolation/morphing from the source banks. This build is currently hardware-usable and stable at `1 kHz` control rate (`48` sample divisor), with MIDI note/gate active and MIDI CC mapping still disabled. Reported side effect: floatable morph timbre is slightly different from the previous rendered-table path.
  Investigation sequence:
  1. `128` rendered-table pass: completed, insufficient glitch reduction.
  2. Braids-style compact-param + audio-rate morph pass: implemented and currently preferred for stability.
  Goal: keep the stability gains while tuning the morph behavior if needed.
- `What to test`: Sweep all `16` waves in Out1 and Out2 for both normal and alt, compare morph tone against the previous rendered-table build, and confirm no regressions in other engines while MIDI note/gate remains active.

### 4. Fix the global VCA
- `Status`: Done
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp), Utility-Pair reference behavior
- `Notes`: Closed in the current build. Keep regression-checking CV2 disconnected = unity and connected CV attenuation whenever the final output path changes.
- `What to test`: Verify CV2 disconnected = unity, connected = expected attenuation law, and compare response against Utility-Pair behavior.

### 5. Revisit `losenge` alt mode purpose
- `Status`: Open
- `Scope`: [prex/losenge/losenge_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/losenge/losenge_solo_engine.cpp), [prex/mtws/engines/losenge_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/losenge_engine.cpp)
- `Notes`: `Y` already provides a continuous tract-size / male-female-style formant shift, so `alt` may be redundant as a female table switch. Decide whether to keep `alt` as a separate vowel table or repurpose it for something more distinct. Maybe try running at f2-f4, not f1-f3
- `What to test`: Compare current `alt` behavior against a repurposed alternative after the solo engine is stabilized, then decide what should carry into integrated `mtws`.

### 6. Check Braids `HARM` and `WMAP` for optimization ideas
- `Status`: Done
- `Scope`: [reference/twists/releases/10_twists/src/braids](/Users/jeff/Toonbox/MTWS/reference/twists/releases/10_twists/src/braids), [prex/cumulus/cumulus_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/cumulus/cumulus_solo_engine.cpp), [prex/mtws/engines/cumulus_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/cumulus_engine.cpp), [prex/floatable/floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp)
- `Notes`: The Braids `WMAP` review informed the current non-wrapping bilinear floatable mapping and the solo rendered-table experiments. Closed after completing this optimization review pass.
- `What to test`: Compare CPU headroom, stability, and any audible tradeoffs after applying candidate optimizations to each engine.

### 7. Try Braids `FOLD` for `bender`
- `Status`: Done
- `Scope`: [reference/twists/releases/10_twists/src/braids](/Users/jeff/Toonbox/MTWS/reference/twists/releases/10_twists/src/braids), [prex/bender/bender_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/bender/bender_solo_engine.cpp), [prex/mtws/engines/bender_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/bender_engine.cpp)
- `Notes`: Evaluated in the solo engine and rolled back. The Utility-Pair-derived fold path is preferred because it produces stronger sine harmonics.
- `What to test`: Compare fold character, aliasing behavior, control feel, and level behavior between the current `bender` implementation and a Braids `FOLD` variant.

### 8. Rethink / Recheck Anti aliasing
- `Status`: Open
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp)
- `Notes`: Even with the polybleb, there is still high frequency aliasing artifacts. Look at a low pass on all outputs option
- `What to test`: Check that high frequency aliasing goes away.

### 9. Sub out on pulse 2
- `Status`: Done
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp)
- `Notes`: Closed for now. The previous "sub out on pulse2" / midi-clock direction is superseded by a different idea.
- `What to test`: N/A for this closed item.

### 10. A bit more volume in cumulus
- `Status`: Open

### 11. Finish I/O
- `Status`: Open
- `Notes`:
    - In:
        - Main : Tuning
        - X : Engine X
        - Y : Engine Y
        - CV1 : Tunning added to main
        - CV2 : VCA in
        - Pulse1 : Alt latch
        - Pulse2 : Slot switch on rising edge
        - Audio1 : Engine X with X knob as attenuator 
        - Audio2 : Engine Y with Y knob as attenuator 
    - Out:
        - CV1 : last midi note
        - CV2 : Midi CC 74 mapped across raw CV range (approx `-6V..+6V`)
        - Pulse1: Midi on/off gate
        - Pulse2: Midi clock (TODO, see item 13)
        - Audio1: Engine out 1
        - Audio2: Engine out 2
    - Midi: 
        - Channel : 1
        - Note on/off + note pitch are active
        - CC 74 -> CV2 is active
        - CC X/Y mapping is disabled in current build

### 12. Maybe add Reese bass to sawsome on alt for pulse width
- `Status`: Done
- `Notes`: Closed. No additional Reese-bass alt path planned right now.

### 13. Add MIDI clock on pulse 2 out
- `Status`: Open
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp), [prex/mtws/core/midi_worker.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/core/midi_worker.cpp)
- `Notes`: Pulse2 out is reserved for MIDI clock instead of the older sub-out idea. Leave it unimplemented in the current pass and add clock generation later.
- `What to test`: Verify clock pulse rate, polarity, and start/stop behavior once MIDI clock output is implemented.

### 14. Try reducing `sawsome` from 7 waves to 5
- `Status`: Open
- `Scope`: [prex/mtws/engines/sawsome_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/sawsome_engine.cpp), [prex/sawsome/sawsome_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/sawsome/sawsome_solo_engine.cpp)
- `Notes`: Check whether dropping `sawsome` from `7` waves/voices to `5` buys enough audio headroom to avoid the remaining glitch risk under heavier control or MIDI activity. Keep the character as close as possible to the current spread and level behavior if this is tried.
- `What to test`: Compare `7` vs `5` waves for CPU stability, glitch resistance, stereo spread, loudness balance, and tone in both normal and alt modes.
