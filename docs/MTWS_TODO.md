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
- `Status`: In Progress
- `Scope`: [prex/floatable/floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp), [prex/mtws/wavetables/floatable_bank_1_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_1_16x256.h), [prex/mtws/wavetables/floatable_bank_2_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_2_16x256.h), [prex/mtws/wavetables/floatable_bank_3_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_3_16x256.h), [prex/mtws/wavetables/floatable_bank_4_16x256.h](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/wavetables/floatable_bank_4_16x256.h)
- `Notes`: The curated browser exports are now checked in as shared `4 x 16 x 256` bank headers, and both the standalone `floatable` target and integrated `mtws` render two finished `256`-sample tables at control rate before the audio core phase-reads them. The remaining work is hardware validation of bank order, stereo usefulness, and whether any wave ordering still needs refinement.
  Investigation plan for the current integrated `mtws` glitch:
  Test 1: single wavetable, no `Y`.
  1. Comment out the second wavetable in integrated `mtws`; disable it without deleting the code.
  2. Comment out the `Y` control in integrated `mtws`; disable it without deleting the code.
  3. Send `Out1` to `Out2`.
  4. Disable `alt` mode without deleting the code.
  Goal: confirm whether the glitch is specifically caused by running two rendered wavetables at once inside integrated `mtws`.
  Test 2: restore the full floatable path with two wavetables and `alt`.
  1. Re-enable both rendered wavetable outputs and `alt` mode.
  2. Try alternating control-rate updates so `X` and `Y` are calculated on separate control ticks: one control tick updates `X`, the next updates `Y`, then repeat.
  Goal: check whether halving the per-tick `X`/`Y` control work removes the glitch even though each axis updates at half the current rate.
- `What to test`: Sweep all `16` waves in Out1 and Out2 for both normal and alt, confirm Alt swaps to the second bank pair cleanly, listen for any weak transitions or clicks that would justify reordering the curated banks, then run the dedicated glitch investigation plan above on integrated `mtws`.

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
- `Status`: Open
- `Scope`: [reference/twists/releases/10_twists/src/braids](/Users/jeff/Toonbox/MTWS/reference/twists/releases/10_twists/src/braids), [prex/cumulus/cumulus_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/cumulus/cumulus_solo_engine.cpp), [prex/mtws/engines/cumulus_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/cumulus_engine.cpp), [prex/floatable/floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp)
- `Notes`: The Braids `WMAP` review already informed the current non-wrapping bilinear floatable mapping and the solo rendered-table experiments. This item stays open for any further reusable patterns that could still help `cumulus` or future floatable bank/tooling work.
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
- `Status`: Blocked
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp)
- `Notes`: Generate a "sub" on pulse out2 that's is 1/2 the main frequency - Update, this should be midi clock rather.
- `What to test`: Check it works

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
        - Pulse2 : Slot switch
        - Audio1 : Engine X with X knob as attenuator 
        - Audio2 : Engine Y with Y knob as attenuator 
    - Out:
        - CV1 : last midi note
        - CV2 : Midi CC 11
        - Pulse1: Midi on/off
        - Pulse2: Sub out
        - Audio1: Engine out 1
        - Audio2: Engine out 2
    - Midi: 
        - Channel : 1
        - Note on/off + note pitch are active
        - CC X/Y mapping is disabled in current build

### 12. Maybe add Reese bass to sawsome on alt for pulse width
