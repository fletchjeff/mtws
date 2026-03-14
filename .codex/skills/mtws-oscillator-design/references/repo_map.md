# Repo Map for Oscillator Design

Use this file to choose the smallest set of repo references that answer the current design question.

## Policy and Constraints
- [../../../../AGENTS.md](../../../../AGENTS.md): workflow, test loop, and realtime rules
- [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md): helper reuse order, Utility-Pair source map, and bootstrap patterns

## Good Existing Engine Anchors
- [../../../../prex/bender/](../../../../prex/bender/): simplest solo pattern for a new oscillator scaffold
- [../../../../prex/sawsome/](../../../../prex/sawsome/): use when voice spread, detune maps, or multi-voice stereo logic matter
- [../../../../prex/mtws/engines/bender_engine.cpp](../../../../prex/mtws/engines/bender_engine.cpp): simplest integrated-slot control/audio split
- [../../../../prex/mtws/engines/din_sum_engine.cpp](../../../../prex/mtws/engines/din_sum_engine.cpp): heavier control-rate caching and hot-path optimization examples

## Docs to Read Selectively
- [../../../../docs/engines/](../../../../docs/engines/): current engine goals, control maps, and test checklists
- [../../../../docs/MTWS_USER_GUIDE.md](../../../../docs/MTWS_USER_GUIDE.md): integrated build behavior that overrides standalone docs when they differ
- [../../../../prex/mtws/engines/bender_engine.cpp](../../../../prex/mtws/engines/bender_engine.cpp): simple integrated optimization patterns
- [../../../../prex/mtws/engines/din_sum_engine.cpp](../../../../prex/mtws/engines/din_sum_engine.cpp): cached-frame and hot-path optimization patterns

## Utility-Pair Files to Check First
- `../../../../AGENT_REFERENCE.md` already points to the most useful siblings in `../Utility-Pair/src/`
- Start with `../Utility-Pair/src/main.cpp`, `delayline.h`, `bernoulligate.h`, `ComputerCard.h`, and `t185_state.h`
