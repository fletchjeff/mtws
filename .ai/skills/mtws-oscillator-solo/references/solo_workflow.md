# Standalone Oscillator Workflow

Follow this order for a new standalone oscillator.

## Files to Add
- `knots/solo_engines/<engine>/main.cpp`
- `knots/solo_engines/<engine>/<engine>_solo_engine.h`
- `knots/solo_engines/<engine>/<engine>_solo_engine.cpp`
- `docs/engines/<engine>.md`

## Files to Update
- `CMakeLists.txt`: add one new `add_knots_target(...)` block and target sources for the solo engine plus `knots/solo_engines/solo_common/solo_control_router.cpp`

## Source Patterns
- Primary starter: [../../../../knots/solo_engines/bender/](../../../../knots/solo_engines/bender/)
- Optional voice-spread reference: [../../../../knots/solo_engines/sawsome/](../../../../knots/solo_engines/sawsome/)
- Shared host scaffold: [../../../../knots/solo_engines/solo_common/solo_app_base.h](../../../../knots/solo_engines/solo_common/solo_app_base.h)
- Shared control mapping: [../../../../knots/solo_engines/solo_common/solo_control_router.cpp](../../../../knots/solo_engines/solo_common/solo_control_router.cpp)

## Implementation Rules
- `Init()` should reset deterministic runtime state.
- `BuildRenderFrame()` should convert control-domain values into a lightweight audio-rate frame.
- `RenderSample()` should stay focused on sample generation and output-domain clamping.
- Prefer `mtws::SineLUT` plus simple integer helpers for the first audible scaffold.

## Optimization References
- If the first scaffold misses timing, recheck [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md) first.
- Then compare [../../../../knots/src/engines/bender_engine.cpp](../../../../knots/src/engines/bender_engine.cpp) and [../../../../knots/src/engines/din_sum_engine.cpp](../../../../knots/src/engines/din_sum_engine.cpp) for concrete `mtws` optimization patterns.
- Recheck [../../../../AGENTS.md](../../../../AGENTS.md) before introducing heavier DSP in `RenderSample()`.

## Build and Test
- Build command: `mkdir -p build && cd build && cmake .. && make -j`
- Flash the standalone `.uf2`
- Verify:
  - pitch response on `Main`
  - `X` and `Y` span useful ranges
  - `Alt` audibly changes the patch
  - outputs remain inside the board range without obvious clipping
