---
name: mtws-oscillator-integration
description: Use when wiring a solo oscillator or DSP core into the six-slot mtws host, including control frames, engine registry, docs, and build and hardware validation.
---

# MTWS Oscillator Integration

Use this skill when a standalone oscillator or DSP core needs to become an integrated `mtws` slot.

## Start Here

1. Read [../../../AGENTS.md](../../../AGENTS.md) and [../../../AGENT_REFERENCE.md](../../../AGENT_REFERENCE.md).
2. Read [references/integration_workflow.md](references/integration_workflow.md).
3. Use the starter templates in [assets/templates/](assets/templates/).
4. Compare first with [../../../prex/mtws/engines/bender_engine.h](../../../prex/mtws/engines/bender_engine.h) and [../../../prex/mtws/engines/bender_engine.cpp](../../../prex/mtws/engines/bender_engine.cpp).

## Workflow

1. Add the new per-engine control frame to `prex/mtws/core/shared_state.h`.
2. Add the new engine header and source under `prex/mtws/engines/`.
3. Wire the engine into `engine_registry.cpp` and any slot-count or slot-order docs affected.
4. Add the new engine source to `CMakeLists.txt`.
5. Update the per-engine doc and `MTWS_USER_GUIDE.md` if integrated behavior changes.
6. Build `mtws`, then flash and run the slot-level hardware smoke tests.

## Integration Rules

- Keep `ControlTick()` focused on control-domain preprocessing.
- Keep `RenderSample()` focused on sample generation using the precomputed frame.
- Preserve shared host behavior from `MTWS_USER_GUIDE.md`.
- If the integrated version diverges from the standalone version for performance, document the reason and keep the sonic intent aligned.

## Validation Rules

- Build the integrated `mtws` target after registry and CMake updates.
- Check slot selection, LED behavior, and any shared panel interactions affected by the new engine.
- Report both build results and hardware observations.
