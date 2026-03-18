# MTWS Skills

Repo-tracked skills for AI coding agents. Each skill lives in its own
directory and is described by a `SKILL.md` file with YAML frontmatter
containing `name`, `description`, `prompt`, and `auto_invoke` fields.

## Available Skills

- **[mtws-oscillator-design](mtws-oscillator-design/SKILL.md)** --
  Turn an oscillator idea into a decision-complete spec before code edits begin.

- **[mtws-oscillator-solo](mtws-oscillator-solo/SKILL.md)** --
  Scaffold a standalone oscillator target under `knots/solo_engines/<engine>/`.

- **[mtws-oscillator-integration](mtws-oscillator-integration/SKILL.md)** --
  Wire an oscillator into the six-slot `mtws` host, registry, docs, and
  validation loop.

## Reference Routing

Before using one of the repo skills, read the repo-root guides first:
- `AGENTS.md`: repo policy, workflow, validation loop, and realtime rules
- `AGENT_REFERENCE.md`: reference routing, helper catalog, and bootstrap patterns

Choose references by task type:
- `reference/utility_pair/`: DSP helpers, curves, filters, delay utilities, and small oscillator blocks
- `ComputerCard.h`: Workshop System platform API, jack behavior, calibrated helpers, and board I/O conventions
- `reference/workshop_computer_examples/`: USB, MIDI, second-core, calibration, sample upload, and web tooling patterns
- `reference/10_twists/`: larger oscillator architecture, interpolation, resources/settings, and USB worker patterns

## Skill Layout

```
.ai/skills/<skill-name>/
  SKILL.md                   # instructions + universal frontmatter
  references/                # supporting docs (optional)
  assets/templates/          # code templates (optional)
```

## Adding a New Skill

1. Create a new directory under `.ai/skills/` with a lowercase hyphenated name.
2. Add a `SKILL.md` with the required frontmatter fields.
3. Add this skill to the list above and to the `Available Repo Skills`
   section in the repo-root `AGENTS.md`.
