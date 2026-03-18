# Integrated Slot Workflow

Follow this order when adding a new `mtws` engine slot.

## Files to Add
- `knots/src/engines/<engine>_engine.h`
- `knots/src/engines/<engine>_engine.cpp`
- `docs/engines/<engine>.md` if the standalone doc does not already exist

## Files to Update
- `knots/src/core/shared_state.h`
- `knots/src/engines/engine_registry.h`
- `knots/src/engines/engine_registry.cpp`
- `CMakeLists.txt`
- `docs/MTWS_USER_GUIDE.md` if the integrated slot list or control map changes

## Source Patterns
- Primary starter: [../../../../knots/src/engines/bender_engine.cpp](../../../../knots/src/engines/bender_engine.cpp)
- Multi-voice reference: [../../../../knots/src/engines/sawsome_engine.cpp](../../../../knots/src/engines/sawsome_engine.cpp)
- Cached-control reference: [../../../../knots/src/engines/din_sum_engine.cpp](../../../../knots/src/engines/din_sum_engine.cpp)
- Host interface: [../../../../knots/src/engines/engine_interface.h](../../../../knots/src/engines/engine_interface.h)

## Integration Checklist
- Add one dedicated control-frame struct for the engine
- Add the engine field to `EngineControlFrame`
- Add the engine class member to `EngineRegistry`
- Add the new slot pointer to the registry slot array in the intended order
- Add the engine source file to the `mtws` target in `CMakeLists.txt`
- Update docs to match the real integrated control map and slot order

## Optimization References
- If the integrated path is too heavy, recheck [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md) first.
- Then compare [../../../../knots/src/engines/bender_engine.cpp](../../../../knots/src/engines/bender_engine.cpp) and [../../../../knots/src/engines/din_sum_engine.cpp](../../../../knots/src/engines/din_sum_engine.cpp) for cached control-frame strategies already used in `mtws`.
- Recheck [../../../../AGENTS.md](../../../../AGENTS.md) before moving any new work into the audio hot path.

## Build and Test
- Build command: `mkdir -p build && cd build && cmake .. && make -j`
- Flash `mtws.uf2`
- Verify:
  - slot selection reaches the new engine
  - the new engine responds to `Main`, `X`, `Y`, and `Alt`
  - shared behaviors from `MTWS_USER_GUIDE.md` still work
  - there are no new clicks, dropouts, or obvious overruns
