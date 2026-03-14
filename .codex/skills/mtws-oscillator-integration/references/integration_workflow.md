# Integrated Slot Workflow

Follow this order when adding a new `mtws` engine slot.

## Files to Add
- `prex/mtws/engines/<engine>_engine.h`
- `prex/mtws/engines/<engine>_engine.cpp`
- `docs/engines/<engine>.md` if the standalone doc does not already exist

## Files to Update
- `prex/mtws/core/shared_state.h`
- `prex/mtws/engines/engine_registry.h`
- `prex/mtws/engines/engine_registry.cpp`
- `CMakeLists.txt`
- `docs/MTWS_USER_GUIDE.md` if the integrated slot list or control map changes

## Source Patterns
- Primary starter: [../../../../prex/mtws/engines/bender_engine.cpp](../../../../prex/mtws/engines/bender_engine.cpp)
- Multi-voice reference: [../../../../prex/mtws/engines/sawsome_engine.cpp](../../../../prex/mtws/engines/sawsome_engine.cpp)
- Cached-control reference: [../../../../prex/mtws/engines/din_sum_engine.cpp](../../../../prex/mtws/engines/din_sum_engine.cpp)
- Host interface: [../../../../prex/mtws/engines/engine_interface.h](../../../../prex/mtws/engines/engine_interface.h)

## Integration Checklist
- Add one dedicated control-frame struct for the engine
- Add the engine field to `EngineControlFrame`
- Add the engine class member to `EngineRegistry`
- Add the new slot pointer to the registry slot array in the intended order
- Add the engine source file to the `mtws` target in `CMakeLists.txt`
- Update docs to match the real integrated control map and slot order

## Optimization References
- If the integrated path is too heavy, recheck [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md) first.
- Then compare [../../../../prex/mtws/engines/bender_engine.cpp](../../../../prex/mtws/engines/bender_engine.cpp) and [../../../../prex/mtws/engines/din_sum_engine.cpp](../../../../prex/mtws/engines/din_sum_engine.cpp) for cached control-frame strategies already used in `mtws`.
- Recheck [../../../../AGENTS.md](../../../../AGENTS.md) before moving any new work into the audio hot path.

## Build and Test
- Build command: `mkdir -p build && cd build && cmake .. && make -j`
- Flash `mtws.uf2`
- Verify:
  - slot selection reaches the new engine
  - the new engine responds to `Main`, `X`, `Y`, and `Alt`
  - shared behaviors from `MTWS_USER_GUIDE.md` still work
  - there are no new clicks, dropouts, or obvious overruns
