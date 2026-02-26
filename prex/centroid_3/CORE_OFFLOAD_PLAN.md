# centroid_3 Core Offload Plan (Steps 1-12)

## Goal
Progress from the current 3-voice additive scaffold to a stable 16-voice implementation where control-rate computation (gains + ratios) is offloaded to core 1, while core 0 remains deterministic for 48k audio rendering.

## Mode Mapping
- `Switch::Up` = alt mode
- `Switch::Middle` = normal mode
- `Switch::Down` = normal mode (same as Middle for now)

## Progress Snapshot (2026-02-26)
- ✅ Step 1 complete: baseline freeze verified on hardware/analyzer.
- ✅ Step 2 complete: `ControlFrame` scaffold added on core 0.
- ✅ Step 3 complete: core-0 double buffer publish/consume pattern added.
- ✅ Step 4 complete: core 1 launched with control-rate loop scaffold.
- ✅ Step 5 complete: core 1 owns frame publication path.
- ✅ Step 6 complete: core 1 computes base pitch + per-voice phase increments and publishes full frame.
- ✅ Step 7 complete: core 0 snapshots `main/x/y/switch`; core 1 consumes snapshot.
- ▶️ Next target: Step 8 (normal mode centroid gain mapping with harmonic frequencies retained).

## Step 1 - Baseline Freeze
- Keep current code unchanged.
- Record baseline analyzer plots at a few fixed pitches.
- Confirm expected peaks for current scaffold behavior.
- Gate to continue: baseline behavior reproducible after reflashing.

## Step 2 - Add ControlFrame Scaffold on Core 0
- Introduce a `ControlFrame` struct:
  - `voice_phase_increment[]`
  - `voice_gain_q12[]`
  - `mix_norm_q12`
- Keep computation and consumption both on core 0.
- Gate to continue: no analyzer difference vs Step 1.

## Step 3 - Add Publish/Consume Double Buffer (Still Core 0 Only)
- Add two `ControlFrame` buffers and a published index.
- Writer updates inactive buffer then flips published index atomically.
- Reader consumes only published complete frame.
- Gate to continue: no glitches under continuous knob movement.

## Step 4 - Bring Up Core 1 Loop (No DSP Migration Yet)
- Launch core 1 with a paced control-rate loop.
- No control frame publishing yet from core 1.
- Gate to continue: stable run, no crashes, no regression.

## Step 5 - Move Gain Calculation to Core 1
- Core 1 computes `voice_gain_q12[]` and publishes frame.
- Core 0 keeps static harmonic ratios.
- Gate to continue: gain motion behaves correctly; system stable under modulation.

## Step 6 - Move Gain + Ratio Calculation to Core 1
- Core 1 computes both:
  - `voice_gain_q12[]`
  - `voice_phase_increment[]` (or ratio-derived increments)
- Core 0 audio loop consumes published frame only.
- Gate to continue: no torn frames, no audible instability.

## Step 7 - Add Control Snapshot Path (Core 0 -> Core 1)
- Core 0 snapshots controls at control rate into shared input state:
  - `main`, `x`, `y`, `switch`
- Core 1 reads snapshots and computes next frame.
- Gate to continue: deterministic behavior; no direct UI reads needed in core 1 DSP path.

## Step 8 - Implement Normal Mode Centroid Gain Mapping
- Keep harmonic ratios (`i+1`) in normal mode.
- Use `X` to set centroid position across partial index.
- Use `Y` to set width/slope (spread) of gain curve around centroid.
- Gate to continue: analyzer shows amplitude envelope moving with X/Y while frequencies stay harmonic.

## Step 9 - Implement Alt Mode Frequency Pull Toward Centroid
- Active only when `Switch::Up`.
- Keep centroid gain mapping active.
- Detune each partial toward centroid frequency using Y as warp amount:
  - `r_harm(i) = i + 1`
  - `r_centroid = 1 + centroid_index`
  - `warp = Y_norm`
  - `r_alt(i) = r_harm(i) + warp * (r_centroid - r_harm(i))`
- Expected behavior:
  - `Y=0` -> harmonic spacing
  - `Y=max` -> partials collapse to centroid frequency
- Gate to continue: analyzer confirms partial frequencies move toward centroid in Up mode.

## Step 10 - Add Smoothing and Safety Bounds
- Add smoothing on controls and/or published frame values.
- Clamp gains and ratios/increments to safe bounds.
- Keep publish atomic.
- Gate to continue: no zipper noise/click bursts during fast modulation.

## Step 11 - Scale Voice Count Incrementally
- Increase voices in stages: `3 -> 4 -> 8 -> 16`.
- Keep same control-frame interface and publish/consume mechanism.
- At each stage:
  - build
  - flash
  - analyzer check
  - modulation stress check
- Gate to continue: each stage stable before increasing voices.

## Step 12 - Long-Run Stress + Validation
- Run continuous modulation test (manual + CV/LFO).
- Validate:
  - no crashes
  - no audio dropouts
  - no frame corruption artifacts
  - mode switching behavior correct
- Finalize if stable for sustained runtime.

## Testing Protocol Per Step
- Build: `cd mtws_additive/build && cmake .. && make -j centroid_3`
- Flash generated `centroid_3.uf2`
- Perform analyzer check + knob/CV movement check
- Only move to next step if step-specific gate passes
