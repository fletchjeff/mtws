# Host Preview (Solo Engines)

This host tool renders the existing solo-engine DSP to a stereo WAV file on macOS/Linux, without touching board `.uf2` builds.

- Board targets remain unchanged.
- Host target is separate: `tools/host_preview`.
- Audio render settings match board timing:
  - sample rate: `48kHz`
  - control updates: every `16` samples

## Build

```bash
cd mtws/tools/host_preview
cmake -S . -B build
cmake --build build -j
```

## Basic Render

```bash
./build/mtws_host_preview \
  --engine cumulus \
  --seconds 10 \
  --main 2300 \
  --x 1600 \
  --y 2800 \
  --alt 0 \
  --out /tmp/cumulus.wav
```

Play it with your preferred player (QuickTime, VLC, DAW import, etc).

## Realtime Playback (macOS)

Stream directly to the default Mac output device (Out1 -> Left, Out2 -> Right):

```bash
./build/mtws_host_preview \
  --realtime \
  --engine cumulus \
  --main 2300 \
  --x 1600 \
  --y 2800 \
  --alt 0
```

Run for a fixed duration:

```bash
./build/mtws_host_preview \
  --realtime \
  --duration 20 \
  --engine floatable \
  --main 2100 \
  --x 3000 \
  --y 900
```

Notes:

- If `--duration` is omitted, playback runs until `Ctrl+C`.
- Realtime mode uses the same render/control timing as offline mode:
  - sample rate: `48kHz`
  - control update divisor: `16`

## Available Engines

- `sawsome`
- `bender`
- `floatable`
- `cumulus`
- `losenge`
- `din_sum`

## Input/Control Arguments

- `--main` in `0..4095`
- `--x` in `0..4095`
- `--y` in `0..4095`
- `--audio1` in `-2048..2047`
- `--audio2` in `-2048..2047`
- `--audio1-connected` in `0|1`
- `--audio2-connected` in `0|1`
- `--alt` in `0|1`

Macro behavior matches solo firmware:

- if audio input is disconnected, macro follows knob directly
- if connected, audio input is clamped to approximately `-5V..+5V`, remapped to `0..4095`, and scaled by the knob as attenuation amount
- with a connected input and knob full CCW, macro stays at `0`
- with a connected input and knob full CW, `-5V -> 0`, `0V -> ~2048`, and `+5V -> 4095`

## Automation CSV

Use `--script` to apply control changes at specific times.

Example:

```csv
time_s,main,x,y,alt,audio1,audio2,audio1_connected,audio2_connected
0.0,2048,0,0,0,0,0,0,0
1.0,2048,4095,0,0,0,0,0,0
2.0,2048,4095,4095,1,0,0,0,0
3.0,2400,3000,1200,1,900,-300,1,1
```

Run with:

```bash
./build/mtws_host_preview --engine floatable --seconds 8 --script ./automation.csv --out /tmp/floatable_auto.wav
```

Notes:

- `time_s` is seconds; you can also use `time_ms`.
- Rows are step updates (state holds until next row).
- Missing columns keep previous values.
