# Repository Guidelines

## Canonical Agent Policy
This file is the single source of truth for agent behavior inside the `mtws` repo.

## Design Philosophy
This is a shared musical instrument, not a reference DSP platform.

- Clarity over cleverness
- Musical usefulness over technical purity
- Curiosity over perfection
- Sharing over gatekeeping

Technical imperfections (aliasing, distortion, roughness) are **not automatically bugs** if they are musically interesting, consistent, controllable, and performant. Do not auto-fix them unless instructed or unless clearly harmful (DC runaway, hard hangs, broken downstream patches). When proposing a "quality" change, state whether the goal is *musical character* or *fidelity*, and why.

Write code that runs, explains itself, and that someone else can open in six months and understand.

## IMPORTANT
- Run git operations only in this repo when the task is about `mtws`.
- Work in small increments and validate through the build/flash test loop.
- Always provide a concrete change plan and get user approval before editing code.
- When debugging a performance issue or regression, comment out code rather than deleting it until the issue is resolved.

## Comment Philosophy
Code should be understandable to a curious non-expert. Many people reading or modifying this code may not have a strong DSP or C++ background.

For every function and non-trivial block, explain:
- **What** the code is doing
- **Why** it is being done
- **What the musical or practical outcome will be**

Example: *"We apply tanh() here to gently limit the feedback level. This prevents runaway amplification and adds a subtle analogue-style saturation."*

Assume someone might open a file in six months to borrow an idea. Clarity is generosity.

## Available Repo Skills
- `mtws-oscillator-design` in `.ai/skills/mtws-oscillator-design/`: use when turning an oscillator idea into a concrete `mtws` spec before code edits.
- `mtws-oscillator-solo` in `.ai/skills/mtws-oscillator-solo/`: use when creating or refactoring a standalone oscillator target under `knots/solo_engines/<engine>/`.
- `mtws-oscillator-integration` in `.ai/skills/mtws-oscillator-integration/`: use when wiring an oscillator into the six-slot `mtws` host, registry, docs, and validation loop.

## Current Project Scope
- Current user-facing behavior is documented in `docs/MTWS_USER_GUIDE.md`.
- Engine-specific docs live in `docs/engines/`.

## Scope and Priorities
- Active firmware work is in `knots/`.
- The integrated host lives in `knots/src/`.
- Standalone solo engines live in `knots/solo_engines/`.
- Shared user and engineering docs live in `docs/`.
- Reference material (Utility-Pair helpers, Workshop Computer examples, 10 Twists) lives in `reference/`.
- Do not edit vendored or reference code such as `../pico-sdk/` or `reference/` unless explicitly requested.

## Reference Routing
- Start with `AGENT_REFERENCE.md` to choose the smallest relevant reference set before reading code broadly.
- Use the smallest matching bucket for the task: `reference/utility_pair/` for reusable DSP helpers, repo-root `ComputerCard.h` for platform behavior, `reference/workshop_computer_examples/` for board-level examples, and `reference/10_twists/` for larger oscillator architecture.
- When a task touches both platform behavior and DSP, read the relevant platform reference first, then the smallest matching DSP reference.

## RP2040 + ComputerCard Constraints
- Audio callback rate is 48kHz; `ProcessSample()` budget is about 20.8µs.
- Keep audio-rate code deterministic and lightweight.
- Prefer integer and fixed-point math; avoid float and dynamic allocation in hot paths. Use `sinf()` not `sin()`, `float` not `double`.
- Use `__not_in_flash_func()` for performance-critical code paths when needed.
- Move heavy non-audio work to core 1 when appropriate.
- Do not run USB MIDI service (`tud_task()`) on the audio core path.

### ProcessSample Overrun Symptoms
If `ProcessSample()` exceeds the ~20µs budget, the ADC/MUX pipeline desyncs. Symptoms include:
- Knob values lock at ~1780–1790
- Input channels permute (wrong knob controls wrong parameter)
- Values drop to zero unexpectedly

These are not software bugs — they indicate the interrupt is taking too long. A 30+µs quantiser lookup has been confirmed to cause this. **Profiling:** toggle a debug GPIO at start/end of `ProcessSample()` and trigger an oscilloscope on pulse length exceeding a threshold, or use `hardware_timer` microsecond timers.

## Current `mtws` Execution Split
- The realtime audio loop ultimately runs in `ComputerCard::AudioWorker()` and calls `ProcessSample()` at `48kHz`.
- The integrated `mtws` host launches a separate core-1 worker from `MTWSApp::ProcessSample()` using `multicore_launch_core1(Core1Entry)`.
- That worker runs `MTWSApp::ControlAndMIDIWorkerCore()`, which handles non-audio work and publishes control frames for the audio path.
- `MIDIWorker::Poll()` calls `tud_task()`, so USB MIDI servicing stays off the audio core path.
- In the integrated host, `ControlTick()` is not per-sample: `kControlDivisor = 48`, so control updates happen every 48 audio samples, about `1kHz`.
- Treat this split as the default `mtws` architecture unless there is a concrete reason to change it.

## Clock Policy
- `knots/src/main.cpp` currently sets the RP2040 SYS clock to `200MHz` at startup using `vreg_set_voltage(VREG_VOLTAGE_1_15)` and `set_sys_clock_khz(200000, true)`.
- This increases CPU headroom only; it does not change the 48kHz audio sample rate or control scheduling constants by itself.
- Treat clocks above 200MHz as experimental and require explicit user approval before enabling them.
- **Note:** 144MHz with audio at 96kHz is known to reduce ADC tonal artifacts. If ADC noise or tonal artifacts become an issue, 144MHz is worth investigating as an alternative, but 200MHz remains the `mtws` default for headroom.

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
- Prefer adapting proven helpers from `reference/utility_pair/` over inventing new ones.
- Before changing input/output semantics, calibration behavior, or MIDI/CV helper usage, check `ComputerCard.h` and the relevant workshop example first.
- Before adding larger oscillator infrastructure, parameter interpolation, or USB worker changes, check `reference/10_twists/` and existing `knots/src/engines/` patterns first.
- Before changing core split, control cadence, or where `ControlTick()` work happens, inspect `knots/src/main.cpp` first.
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
- `knots/src/`: integrated multi-engine host firmware
- `knots/solo_engines/`: standalone solo engine targets and shared solo scaffolding
- `reference/`: read-only reference material (Utility-Pair helpers, Workshop Computer examples, 10 Twists)
- `docs/`: user guides, engine docs, plans, and optimization notes
- `tools/`: host-side tools and utilities
- `.ai/skills/`: repo-tracked skills for oscillator design and implementation

Keep generated artifacts in `build/` directories and do not commit them.

## Build, Test, and Development Commands
- `mkdir -p build && cd build && cmake .. && make -j`

### Build System Gotchas
- **`PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64` is essential.** Without it, code works on first flash but **fails after reset**. For Arduino: `#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64` before includes.
- **`pico_set_binary_type(name copy_to_ram)` is strongly recommended.** Eliminates flash cache miss timing jitter. Use for all programs that fit in ~264KB. For larger programs, use `__not_in_flash_func()` on hot functions instead.
- **Recommended compiler flags:** `-Wdouble-promotion -Wfloat-conversion -Wall -Wextra`. `-Wdouble-promotion` catches accidental doubles (which are software-emulated and very slow).
- **`pico_enable_stdio_usb(name 0)` must be OFF** — USB stdio interferes with the normalisation probe on GPIO4.
- **`target_link_options(name PRIVATE -Wl,--print-memory-usage)`** — always enable to monitor RAM/flash usage during builds.

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

## AI Etiquette
- **Confirm understanding before starting non-trivial work.** Restate the goal, scope, constraints, and any assumptions, then wait for confirmation. For small, unambiguous changes (a one-line fix, a rename) this is unnecessary; use judgement.
- **LLM code may invent APIs** (e.g. `LED::L1`, which does not exist). Always verify against `ComputerCard.h` before using any API call.
- **If you can't explain the code, don't submit it.** State which parts are AI-generated and confirm it builds.
- **Verify controls match docs.** After implementing, check that knob/switch/CV behavior matches the engine doc and user guide.

## Web Editor Conventions
Many cards ship with a browser editor for configuration/presets that talks to the Workshop Computer over USB via SysEx. When a card has one:

- **Transport:** Prefer WebMIDI + SysEx (widest Chrome-family support, works on some Android). Use WebSerial only when needed (Chrome desktop only). Neither works reliably on iOS — document this on the page.
- **Ship as a single self-contained static HTML file.** Use a predictable path, e.g. `web_config/<card>.html`.
- **Firmware is the source of truth.** The editor must not invent ranges or enums. Parameters must be versioned with stable IDs so settings survive firmware updates.
- **Separate Apply (changes running behaviour) from Save to card (persists across reset).**
- **Safety:** Any setting that can cause very loud output or unstable behaviour needs a warning/confirm.
- **Connection guidance:** The page must include "Use a USB-C data cable" and "Close Serial Monitor/other apps".
- **Minimum PR checklist:** Loads as static HTML → Connect → Read → Apply → Save → Reset → Read matches → Disconnect/reconnect is clean.

## Agent Reference File
For helper catalogs, reference routing, reuse patterns, and oscillator workflow pointers, consult:
- `AGENT_REFERENCE.md`
- `.ai/skills/README.md`
