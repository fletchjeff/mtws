# Repository Guidelines

## Canonical Agent Policy
This file is the single source of truth for agent behavior inside the `mtws` repo.

## IMPORTANT
- This policy applies to work inside `mtws/`. Leave the workspace-root policy in place for wider multi-repo tasks.
- Run git operations only in this repo when the task is about `mtws`.
- Work in small increments and validate through the build/flash test loop.
- Always provide a concrete change plan and get user approval before editing code.
- All code written should be well commented. Every function should have an explanation of what it does and be descriptive about the inputs and outputs.

## Available Repo Skills
- `mtws-oscillator-design` in `.codex/skills/mtws-oscillator-design/`: use when turning an oscillator idea into a concrete `mtws` spec before code edits.
- `mtws-oscillator-solo` in `.codex/skills/mtws-oscillator-solo/`: use when creating or refactoring a standalone oscillator target under `prex/<engine>/`.
- `mtws-oscillator-integration` in `.codex/skills/mtws-oscillator-integration/`: use when wiring an oscillator into the six-slot `mtws` host, registry, docs, and validation loop.

## Current Project Scope
- Current user-facing behavior is documented in `docs/MTWS_USER_GUIDE.md`.
- Engine-specific docs live in `docs/engines/`.

## Scope and Priorities
- Active firmware work is in `prex/`.
- Shared user and engineering docs live in `docs/`.
- Use `../Utility-Pair/` as a reference library for proven helpers and DSP patterns.
- Use `../Workshop_Computer/` for documentation and examples lookup only unless explicitly asked to edit it.
- Do not edit vendored or reference code such as `../pico-sdk/` or any `reference/` subtree unless explicitly requested.

## RP2040 + ComputerCard Constraints
- Audio callback rate is 48kHz; `ProcessSample()` budget is about 20.8us.
- Keep audio-rate code deterministic and lightweight.
- Prefer integer and fixed-point math; avoid float and dynamic allocation in hot paths.
- Use `__not_in_flash_func()` for performance-critical code paths when needed.
- Move heavy non-audio work to core 1 when appropriate.
- Do not run USB MIDI service (`tud_task()`) on the audio core path.

## Clock Policy
- `prex/mtws/main.cpp` currently sets the RP2040 SYS clock to `200MHz` at startup using `vreg_set_voltage(VREG_VOLTAGE_1_15)` and `set_sys_clock_khz(200000, true)`.
- This increases CPU headroom only; it does not change the 48kHz audio sample rate or control scheduling constants by itself.
- Treat clocks above 200MHz as experimental and require explicit user approval before enabling them.

## Common I/O Ranges
- `KnobVal(...)`: `0..4095`
- Audio and CV sample domain for I/O helpers: roughly `+-2048`
- Switch states: `Up`, `Middle`, `Down`

## Implementation Workflow
1. Plan first: list files, functions, behavior changes, and test approach.
2. Wait for user approval.
3. Implement one focused change.
4. Build and report results.
5. Flash `.uf2`, run hardware smoke tests, and report observed behavior.

## Helper-First Policy
- Before adding new DSP or math helpers, check `AGENT_REFERENCE.md` first.
- Prefer adapting proven helpers from `../Utility-Pair/src/` over inventing new ones.
- If not reusing a helper, explain why.
- Keep ports minimal and preserve realtime safety.

## Required Technical Explanations
For non-obvious constants, shifts, LUT sizes, and fixed-point formats, explain:
- what it represents
- why it was chosen
- one alternative considered
- CPU vs memory tradeoff

For mapping and math code such as pitch, frequency, interpolation, and scaling, explain:
- formula in plain language
- endpoint behavior
- quantization and rounding behavior, including fencepost effects where relevant

Use this structure in technical change summaries:
- `What changed`
- `Why`
- `Tradeoffs`
- `What to test`

## Project Structure
This repo contains the active `mtws` firmware plus supporting tools and docs:
- `prex/`: integrated and standalone firmware targets
- `docs/`: user guides, engine docs, plans, and optimization notes
- `tools/`: host-side tools and utilities
- `.codex/skills/`: repo-tracked skills for oscillator design and implementation

Treat archived planning material outside this repo as optional historical context, not as a required dependency for normal `mtws` work.

Keep generated artifacts in `build/` directories and do not commit them.

## Build, Test, and Development Commands
- `mkdir -p build && cd build && cmake .. && make -j`
- `cd ../Utility-Pair && mkdir -p build && cd build && cmake .. && make -j`
- `cd ../Workshop_Computer && npm ci --prefix tools/sitegen && npm run build --prefix tools/sitegen`
- `cd ../Workshop_Computer && python .github/scripts/update-readme.py`

## Coding Style and Naming
- Use C++17 for firmware projects.
- Match local style in each file.
- Prefer descriptive lowercase helper and file names.
- Keep `ProcessSample()` fast and deterministic.

## Testing Guidelines
There is no automated unit-test suite in `mtws`.
- Do a clean build for each relevant target.
- Flash `.uf2` and run hardware smoke tests for impacted paths.
- Document manual test steps and observed results in commits or PR notes.

## Commit and PR Guidelines
- Use short, imperative, specific commit messages.
- Keep commits scoped to one logical change.
- Include purpose, touched paths, build commands run, and hardware validation notes.

## Agent Reference File
For helper catalogs, reuse patterns, and oscillator workflow pointers, consult:
- `AGENT_REFERENCE.md`
