# Input Monitor CLI

Host-side terminal UI for the USB serial stream produced by the `input_monitor`
firmware target.

## What it shows

- Current, min, and max raw counts for `AudioIn1` and `AudioIn2`
- Current, min, and max raw counts for `CVIn1` and `CVIn2`
- Current state and rising-edge counts for `PulseIn1` and `PulseIn2`
- Jack-detect status for all six input jacks

The display updates in place rather than scrolling.

## Setup

```bash
cd mtws/tools/input_monitor_cli
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Run

Flash the firmware tool first:

```bash
cp ../../build/input_monitor.uf2 /Volumes/RPI-RP2/
```

Then run the CLI:

```bash
source .venv/bin/activate
python main.py
```

If auto-detect does not find the serial port, pass it explicitly:

```bash
python main.py --port /dev/cu.usbmodem1234561
```

Useful options:

- `--port <path>`: use a specific serial device
- `--baud <rate>`: serial baud rate, default `115200`
- `--hold-seconds <float>`: how long analogue min/max values stay visible, default `2.0`
- `--no-color`: disable ANSI colors

Exit with `Ctrl+C`.
