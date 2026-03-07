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
