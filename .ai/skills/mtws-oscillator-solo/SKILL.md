---
name: mtws-oscillator-solo
description: >-
  Scaffold a standalone mtws oscillator. Use when creating or refactoring a
  standalone oscillator target in mtws, including knots/solo_engines/<engine>/
  scaffolding, solo_common wiring, docs, and the first build and hardware validation loop.
prompt: >-
  Use this skill to scaffold a standalone oscillator target in this mtws repo.
auto_invoke: true
---

# MTWS Oscillator Solo

Use this skill when the goal is to get a standalone oscillator running under `knots/solo_engines/<engine>/` using `knots/solo_engines/solo_common/`.

## Start Here

1. Read [../../../AGENTS.md](../../../AGENTS.md) and [../../../AGENT_REFERENCE.md](../../../AGENT_REFERENCE.md).
2. Read [references/solo_workflow.md](references/solo_workflow.md).
3. Use the starter templates in [assets/templates/](assets/templates/).
4. Compare with [../../../knots/solo_engines/bender/](../../../knots/solo_engines/bender/) first, then read [../../../knots/solo_engines/sawsome/](../../../knots/solo_engines/sawsome/) only if multi-voice spread or stereo gain maps matter.

## Workflow

1. Pick the engine name and class name.
2. Scaffold `knots/solo_engines/<engine>/main.cpp`, `<engine>_solo_engine.h`, and `<engine>_solo_engine.cpp`.
3. Add the standalone target to `CMakeLists.txt`.
4. Add the engine doc under `docs/engines/`.
5. Build the standalone target.
6. Flash and run the first hardware smoke test before integrating it into the six-slot host.

## Template Rules

- The bundled templates are intentionally small and are based on the lighter `bender` structure.
- Keep control mapping work in `BuildRenderFrame()`.
- Keep audio-rate work in `RenderSample()`.
- If you need more advanced patterns, inspect `knots/solo_engines/sawsome/` for voice maps, `reference/utility_pair/` for reusable DSP blocks, `ComputerCard.h` for platform behavior, and `reference/10_twists/` for larger oscillator/interpolation ideas.

## Validation Rules

- Do a repo build after wiring the target into `CMakeLists.txt`.
- Report the exact target built and the hardware behavior observed.
- If the first prototype is not realtime-safe, use the optimization references before adding new complexity.
