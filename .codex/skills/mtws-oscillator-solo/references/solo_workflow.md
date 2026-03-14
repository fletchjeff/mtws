# Standalone Oscillator Workflow

Follow this order for a new standalone oscillator.

## Files to Add
- `prex/<engine>/main.cpp`
- `prex/<engine>/<engine>_solo_engine.h`
- `prex/<engine>/<engine>_solo_engine.cpp`
- `docs/engines/<engine>.md`

## Files to Update
- `CMakeLists.txt`: add one new `add_prex_target(...)` block and target sources for the solo engine plus `prex/solo_common/solo_control_router.cpp`

## Source Patterns
- Primary starter: [../../../../prex/bender/](../../../../prex/bender/)
- Optional voice-spread reference: [../../../../prex/sawsome/](../../../../prex/sawsome/)
- Shared host scaffold: [../../../../prex/solo_common/solo_app_base.h](../../../../prex/solo_common/solo_app_base.h)
- Shared control mapping: [../../../../prex/solo_common/solo_control_router.cpp](../../../../prex/solo_common/solo_control_router.cpp)

## Implementation Rules
- `Init()` should reset deterministic runtime state.
- `BuildRenderFrame()` should convert control-domain values into a lightweight audio-rate frame.
- `RenderSample()` should stay focused on sample generation and output-domain clamping.
- Prefer `mtws::SineLUT` plus simple integer helpers for the first audible scaffold.

## Optimization References
- If the first scaffold misses timing, recheck [../../../../AGENT_REFERENCE.md](../../../../AGENT_REFERENCE.md) first.
- Then compare [../../../../prex/mtws/engines/bender_engine.cpp](../../../../prex/mtws/engines/bender_engine.cpp) and [../../../../prex/mtws/engines/din_sum_engine.cpp](../../../../prex/mtws/engines/din_sum_engine.cpp) for concrete `mtws` optimization patterns.
- Recheck [../../../../AGENTS.md](../../../../AGENTS.md) before introducing heavier DSP in `RenderSample()`.

## Build and Test
- Build command: `mkdir -p build && cd build && cmake .. && make -j`
- Flash the standalone `.uf2`
- Verify:
  - pitch response on `Main`
  - `X` and `Y` span useful ranges
  - `Alt` audibly changes the patch
  - outputs remain inside the board range without obvious clipping
