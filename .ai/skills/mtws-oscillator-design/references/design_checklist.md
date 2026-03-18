# Design Checklist

Use this checklist as the output shape for design-only tasks.

## Required Sections
- Goal and sonic intent
- Control map for `Main`, `X`, `Y`, `Alt`, `Out1`, and `Out2`
- Reference-routing summary
- Utility-Pair helper or existing-engine reuse plan
- Control-rate versus audio-rate split
- Hot-path risks in `ProcessSample()`
- Required docs to add or update
- Build target or targets to validate
- Hardware smoke tests

## Reuse Questions
Answer these explicitly:
1. Which reference bucket did you check first, and why?
2. Which helper or pattern from `reference/utility_pair/`, `ComputerCard.h`, `reference/workshop_computer_examples/`, `reference/10_twists/`, or an existing `mtws` engine did you check first?
3. Which helper or pattern will you reuse or adapt?
4. If not reusing, why not?

## Performance Questions
Answer these explicitly:
1. Which math stays at audio rate?
2. Which math can move to control rate?
3. Which work can move off the audio core entirely?
4. Are there divisions, modulo operations, or avoidable `int64_t` multiplies in the hot path?
5. Is `__not_in_flash_func()` needed for any new path?

## Validation Questions
Answer these explicitly:
1. Which standalone build proves the first prototype works?
2. Which integrated build proves the slot wiring works?
3. Which hardware tests confirm the control map and sonic behavior?
