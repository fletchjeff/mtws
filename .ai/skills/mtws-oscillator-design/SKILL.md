---
name: mtws-oscillator-design
description: >-
  Spec a new mtws oscillator safely. Use when designing or reviewing a new
  oscillator, including sonic intent, control map, Utility-Pair helper reuse,
  realtime safety, docs, and validation before implementation.
prompt: >-
  Use this skill to turn an oscillator idea into a buildable mtws spec.
auto_invoke: true
---

# MTWS Oscillator Design

Use this skill when the task is to turn an oscillator idea into a decision-complete `mtws` spec before code edits begin.

## Start Here

1. Read [../../../AGENTS.md](../../../AGENTS.md) for repo policy and validation expectations.
2. Read [../../../AGENT_REFERENCE.md](../../../AGENT_REFERENCE.md) for helper reuse rules and bootstrap patterns.
3. Read [references/repo_map.md](references/repo_map.md) to choose the minimum relevant engine examples.
4. Read [references/design_checklist.md](references/design_checklist.md) and structure the output around those sections.

## Workflow

1. Define the sonic goal in plain language.
2. Choose the control map for `Main`, `X`, `Y`, `Alt`, `Out1`, and `Out2`.
3. Identify which helper or pattern from `reference/utility_pair/`, `ComputerCard.h`, `reference/workshop_computer_examples/`, `reference/10_twists/`, or an existing `mtws` engine should be reused first.
4. Split work into control-rate precomputation and audio-rate rendering.
5. Call out hot-path risks and which optimization notes matter.
6. End with the exact build and hardware validation steps needed for the first implementation pass.

## Output Expectations

A good result from this skill includes:
- sonic intent
- control map
- helper reuse plan
- reference-routing summary
- hot-path risk list
- documentation impact
- build and hardware test checklist

## Scope Rules

- Prefer the simplest comparable engine as the primary pattern.
- Do not invent new helpers until `AGENT_REFERENCE.md` and the referenced engine patterns have been checked.
- Keep the design grounded in `mtws` constraints: `48kHz`, deterministic audio loop, integer math preferred, no dynamic allocation in the hot path.
- If the oscillator is likely to need both a standalone prototype and integrated slot work, design both paths up front so they can share the same DSP core.
