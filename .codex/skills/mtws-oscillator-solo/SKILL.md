---
name: mtws-oscillator-solo
description: Use when creating or refactoring a standalone oscillator target in mtws, including prex/<engine> scaffolding, solo_common wiring, docs, and the first build and hardware validation loop.
---

# MTWS Oscillator Solo

Use this skill when the goal is to get a standalone oscillator running under `prex/<engine>/` using `prex/solo_common/`.

## Start Here

1. Read [../../../AGENTS.md](../../../AGENTS.md) and [../../../AGENT_REFERENCE.md](../../../AGENT_REFERENCE.md).
2. Read [references/solo_workflow.md](references/solo_workflow.md).
3. Use the starter templates in [assets/templates/](assets/templates/).
4. Compare with [../../../prex/bender/](../../../prex/bender/) first, then read [../../../prex/sawsome/](../../../prex/sawsome/) only if multi-voice spread or stereo gain maps matter.

## Workflow

1. Pick the engine name and class name.
2. Scaffold `prex/<engine>/main.cpp`, `<engine>_solo_engine.h`, and `<engine>_solo_engine.cpp`.
3. Add the standalone target to `CMakeLists.txt`.
4. Add the engine doc under `docs/engines/`.
5. Build the standalone target.
6. Flash and run the first hardware smoke test before integrating it into the six-slot host.

## Template Rules

- The bundled templates are intentionally small and are based on the lighter `bender` structure.
- Keep control mapping work in `BuildRenderFrame()`.
- Keep audio-rate work in `RenderSample()`.
- If you need more advanced patterns, inspect `prex/sawsome/` for voice maps or `../Utility-Pair/src/` for reusable DSP blocks.

## Validation Rules

- Do a repo build after wiring the target into `CMakeLists.txt`.
- Report the exact target built and the hardware behavior observed.
- If the first prototype is not realtime-safe, use the optimization references before adding new complexity.
