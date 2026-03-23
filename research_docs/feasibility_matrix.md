# Feasibility Matrix

Score meaning:

- `1` = poor fit / high risk / weak value
- `3` = workable with care
- `5` = strong fit / high value / low risk

Risk columns are scored in the opposite direction:

- `1` = low risk
- `5` = high risk

| Technique | Distinctiveness | Hot-path cost risk | Memory/state risk | Alias/DC risk | Control intuitiveness | Repo reuse fit | Verdict | Notes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Phase distortion / phaseshaping | 4 | 2 | 1 | 3 | 5 | 5 | Selected | Strong first build: phase warp + LUT fits current engine patterns. |
| WPPM | 5 | 3 | 1 | 4 | 4 | 4 | Selected | More complex than PD, but still mostly phase math and LUT work. |
| Scanned synthesis | 5 | 4 | 4 | 3 | 2 | 2 | Deferred | Two-rate state evolution is interesting but expensive to make musical fast. |
| Karplus-Strong / digital waveguide | 4 | 2 | 3 | 2 | 5 | 5 | Selected | Delay-line helper and Utility-Pair precedent make this very practical. |
| Modal synthesis | 4 | 3 | 3 | 2 | 4 | 4 | Selected | Keep only as a compact fixed mode bank with precomputed data. |
| Pulsar synthesis | 5 | 4 | 2 | 4 | 3 | 2 | Selected | Worth trying if scoped as a periodic oscillator, not a general granular engine. |
| Chaotic / nonlinear dynamical oscillators | 5 | 5 | 2 | 5 | 2 | 1 | Abandoned | Too much instability and edge-behavior risk for this batch. |
| Feedback AM / nonlinear resonator variants | 4 | 2 | 1 | 4 | 4 | 4 | Selected | Low state cost and distinctive timbre, but needs explicit DC/stability management. |

## Practical Reading Of The Scores

## Best immediate bets

- `Phase distortion / phaseshaping`
- `Karplus-Strong / digital waveguide`
- `Feedback AM`

These are the strongest "can probably become a solo engine without weeks of
infrastructure work" candidates.

## Most interesting but slightly riskier

- `WPPM`
- `Modal synthesis`
- `Pulsar synthesis`

These stay in the six because the sonic payoff looks worth it, but each needs a
scope guard:

- WPPM must stay on a small, windowed, harmonic ratio set at first.
- Modal must use a fixed bank, not general object solving.
- Pulsar must behave like a periodic oscillator, not a CPU-hungry cloud engine.

## Not good first-wave bets

- `Scanned synthesis`
- `Chaotic / nonlinear dynamical oscillators`

Both are musically compelling on paper, but they are the least aligned with the
current MTWS constraints and the most likely to turn into research projects
instead of playable first prototypes.

## Selected 6 For The Future 12-Engine Host

1. Phase distortion / phaseshaping
2. WPPM
3. Karplus-Strong / digital waveguide
4. Feedback AM / nonlinear resonator variants
5. Modal synthesis
6. Pulsar synthesis
