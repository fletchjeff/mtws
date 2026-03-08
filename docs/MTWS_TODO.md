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

### 1. sawsome triangle glitches in `mtws`
- `Status`: Open
- `Scope`: [prex/mtws/engines/sawsome_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/sawsome_engine.cpp)
- `Notes`: Triangle wave in `mtws` glitches. Likely needs lower CPU load or a cheaper triangle path than the current implementation.
- `What to test`: Compare normal vs alt in standalone `sawsome` and integrated `mtws`, then stress-test with high pitch and wide spread.
- Alternative idea for Alt is a Reese bass implementation.

### 2. Try 14 partials in cumulus
- `Status`: Open
- `Scope`: [prex/mtws/engines/cumulus_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/cumulus_engine.cpp)
- `Notes`: Evaluate whether reducing from `16` partials to `14` improves headroom and removes artifacts without losing the intended character.
- `What to test`: Build and hardware compare `16` vs `14` partials for CPU stability, spectral balance, and perceived loudness.

### 3. Better wavetables for floatable
- `Status`: Open
- `Scope`: [prex/floatable/wavetable_data.h](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/wavetable_data.h), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp)
- `Notes`: Current wavetable set should be reviewed and replaced or expanded with stronger source material.
- `What to test`: Sweep the full table set, listen for bland/redundant tables, and compare any replacement bank on hardware.

### 4. Fix the global VCA
- `Status`: Open
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp), Utility-Pair reference behavior
- `Notes`: Current global VCA behavior needs comparison with Utility-Pair and likely correction in the final output scaling path.
- `What to test`: Verify CV2 disconnected = unity, connected = expected attenuation law, and compare response against Utility-Pair behavior.

### 5. Revisit `losenge` alt mode purpose
- `Status`: Open
- `Scope`: [prex/losenge/losenge_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/losenge/losenge_solo_engine.cpp), [prex/mtws/engines/losenge_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/losenge_engine.cpp)
- `Notes`: `Y` already provides a continuous tract-size / male-female-style formant shift, so `alt` may be redundant as a female table switch. Decide whether to keep `alt` as a separate vowel table or repurpose it for something more distinct. Maybe try running at f2-f4, not f1-f3
- `What to test`: Compare current `alt` behavior against a repurposed alternative after the solo engine is stabilized, then decide what should carry into integrated `mtws`.

### 6. Check Braids `HARM` and `WMAP` for optimization ideas
- `Status`: Open
- `Scope`: [reference/twists/releases/10_twists/src/braids](/Users/jeff/Toonbox/MTWS/reference/twists/releases/10_twists/src/braids), [prex/cumulus/cumulus_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/cumulus/cumulus_solo_engine.cpp), [prex/mtws/engines/cumulus_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/cumulus_engine.cpp), [prex/floatable/floatable_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/floatable/floatable_solo_engine.cpp), [prex/mtws/engines/floatable_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/floatable_engine.cpp)
- `Notes`: Review Braids `HARM` and `WMAP` implementations for reusable CPU-saving patterns, table strategies, or control-rate optimizations that could improve `cumulus` and `floatable`.
- `What to test`: Compare CPU headroom, stability, and any audible tradeoffs after applying candidate optimizations to each engine.

### 7. Try Braids `FOLD` for `bender`
- `Status`: Open
- `Scope`: [reference/twists/releases/10_twists/src/braids](/Users/jeff/Toonbox/MTWS/reference/twists/releases/10_twists/src/braids), [prex/bender/bender_solo_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/bender/bender_solo_engine.cpp), [prex/mtws/engines/bender_engine.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/engines/bender_engine.cpp)
- `Notes`: Evaluate whether the Braids `FOLD` algorithm is a better fit for `bender` than the current Utility-Pair-derived fold path.
- `What to test`: Compare fold character, aliasing behavior, control feel, and level behavior between the current `bender` implementation and a Braids `FOLD` variant.

### 8. Rethink / Recheck Anti aliasing
- `Status`: Open
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp)
- `Notes`: Even with the polybleb, there is still high frequency aliasing artifacts. Look at a low pass on all outputs option
- `What to test`: Check that high frequency aliasing goes away.

### 9. Sub out on pulse 2
- `Status`: Open
- `Scope`: [prex/mtws/main.cpp](/Users/jeff/Toonbox/MTWS/mtws/prex/mtws/main.cpp)
- `Notes`: Generate a "sub" on pulse out2 that's is 1/2 the main frequency
- `What to test`: Check it works