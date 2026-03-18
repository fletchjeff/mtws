# {{engine_name}}

## Goal + Sonic Intent
Describe the sound, musical role, and why this oscillator belongs in `mtws`.

## Control Map
- Main: pitch
- X: describe macro `X`
- Y: describe macro `Y`
- Z Up: Alt behavior
- Z Middle: Normal behavior
- Z Down: ignored in standalone
- Out1: describe output
- Out2: describe output

## DSP Block Diagram
- Normal: describe the signal flow
- Alt: describe the alternate signal flow

## Utility-Pair Reuse
- List the helper or pattern checked first in `reference/utility_pair/`
- State what was reused or adapted
- If nothing was reused, explain why

## CPU-Risk Points
- Note expensive math, lookups, voice counts, filters, or feedback paths

## Milestone Steps
1. Get the scaffold audible and stable.
2. Replace placeholder DSP with the intended oscillator path.
3. Tighten the hot path after the first hardware pass.

## Hardware Test Checklist
- Verify pitch range and tuning behavior.
- Verify `X` and `Y` endpoints.
- Verify Alt mode.
- Verify outputs stay inside range without obvious clipping.
