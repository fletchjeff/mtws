#!/usr/bin/env python3
"""Render the mtws input monitor USB serial stream as an in-place terminal UI."""

from __future__ import annotations

import argparse
import dataclasses
import glob
import sys
import time
from typing import Iterable

import serial


EXPECTED_HEADER = [
    "seq",
    "a1",
    "a1_min",
    "a1_max",
    "a2",
    "a2_min",
    "a2_max",
    "cv1",
    "cv1_min",
    "cv1_max",
    "cv2",
    "cv2_min",
    "cv2_max",
    "p1",
    "p1_edges",
    "p2",
    "p2_edges",
    "c_a1",
    "c_a2",
    "c_cv1",
    "c_cv2",
    "c_p1",
    "c_p2",
]

COLOR_RESET = "\x1b[0m"
COLOR_DIM = "\x1b[2m"
COLOR_GREEN = "\x1b[32m"
COLOR_YELLOW = "\x1b[33m"
COLOR_CYAN = "\x1b[36m"
COLOR_BOLD = "\x1b[1m"


@dataclasses.dataclass
class Snapshot:
    """Store one parsed serial report.

    Inputs:
    - One tab-separated line from the `input_monitor` firmware stream.
    Output:
    - A typed record used by the terminal renderer.
    """

    sequence: int
    audio1: int
    audio1_min: int
    audio1_max: int
    audio2: int
    audio2_min: int
    audio2_max: int
    cv1: int
    cv1_min: int
    cv1_max: int
    cv2: int
    cv2_min: int
    cv2_max: int
    pulse1: int
    pulse1_edges: int
    pulse2: int
    pulse2_edges: int
    connected_audio1: int
    connected_audio2: int
    connected_cv1: int
    connected_cv2: int
    connected_pulse1: int
    connected_pulse2: int


@dataclasses.dataclass
class PeakHold:
    """Track one host-side held min/max pair for an analogue channel.

    Inputs:
    - Successive `Snapshot` objects from the firmware stream.
    - A hold duration configured from the CLI.
    Output:
    - A min/max pair that remains visible longer than the device's report window.
    """

    minimum: int
    maximum: int
    min_expires_at: float
    max_expires_at: float


@dataclasses.dataclass
class HeldSnapshot:
    """Store one snapshot plus host-side held min/max values for analogue channels.

    Inputs:
    - The latest firmware snapshot.
    - Peak-hold state computed in the host CLI.
    Output:
    - A renderer-friendly record for the terminal dashboard.
    """

    sequence: int
    audio1: int
    audio1_min: int
    audio1_max: int
    audio2: int
    audio2_min: int
    audio2_max: int
    cv1: int
    cv1_min: int
    cv1_max: int
    cv2: int
    cv2_min: int
    cv2_max: int
    pulse1: int
    pulse1_edges: int
    pulse2: int
    pulse2_edges: int
    connected_audio1: int
    connected_audio2: int
    connected_cv1: int
    connected_cv2: int
    connected_pulse1: int
    connected_pulse2: int


def build_argument_parser() -> argparse.ArgumentParser:
    """Create the command-line parser.

    Inputs:
    - None.
    Output:
    - An `ArgumentParser` configured for port selection and display options.
    """

    parser = argparse.ArgumentParser(
        description="In-place terminal viewer for the mtws input monitor USB serial stream."
    )
    parser.add_argument("--port", help="Serial device path. Defaults to auto-detect on supported platforms.")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate. Default: 115200.")
    parser.add_argument(
        "--timeout",
        type=float,
        default=0.25,
        help="Serial read timeout in seconds. Default: 0.25.",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI colors if your terminal does not support them.",
    )
    parser.add_argument(
        "--hold-seconds",
        type=float,
        default=2.0,
        help="How long analogue min/max values stay visible in the CLI. Default: 2.0.",
    )
    return parser


def colorize(text: str, color_code: str, enabled: bool) -> str:
    """Wrap one string in ANSI color codes when color output is enabled.

    Inputs:
    - `text`: plain text to display.
    - `color_code`: ANSI prefix for the desired color/style.
    - `enabled`: whether ANSI coloring should be applied.
    Output:
    - The original text, optionally wrapped in ANSI escape sequences.
    """

    if not enabled:
        return text
    return f"{color_code}{text}{COLOR_RESET}"


def auto_detect_port() -> str | None:
    """Look for a likely USB CDC serial device path for the RP2040 board.

    Inputs:
    - None.
    Output:
    - A best-guess serial device path, or `None` if nothing suitable was found.
    """

    patterns = [
        "/dev/cu.usbmodem*",
        "/dev/tty.usbmodem*",
        "/dev/ttyACM*",
        "/dev/ttyUSB*",
    ]
    matches: list[str] = []
    for pattern in patterns:
        matches.extend(sorted(glob.glob(pattern)))
    return matches[0] if matches else None


def parse_snapshot_line(line: str) -> Snapshot | None:
    """Parse one line from the device stream.

    Inputs:
    - `line`: one decoded UTF-8 line from the USB serial stream.
    Output:
    - A `Snapshot` for data lines, or `None` for blank/header/malformed lines.
    """

    stripped = line.strip()
    if not stripped:
        return None

    fields = stripped.split("\t")
    if fields == EXPECTED_HEADER:
        return None
    if len(fields) != len(EXPECTED_HEADER):
        return None

    try:
        values = [int(field) for field in fields]
    except ValueError:
        return None

    return Snapshot(
        sequence=values[0],
        audio1=values[1],
        audio1_min=values[2],
        audio1_max=values[3],
        audio2=values[4],
        audio2_min=values[5],
        audio2_max=values[6],
        cv1=values[7],
        cv1_min=values[8],
        cv1_max=values[9],
        cv2=values[10],
        cv2_min=values[11],
        cv2_max=values[12],
        pulse1=values[13],
        pulse1_edges=values[14],
        pulse2=values[15],
        pulse2_edges=values[16],
        connected_audio1=values[17],
        connected_audio2=values[18],
        connected_cv1=values[19],
        connected_cv2=values[20],
        connected_pulse1=values[21],
        connected_pulse2=values[22],
    )


def clamp_index(value: int, width: int) -> int:
    """Map one signed 12-bit count into a bar position.

    Inputs:
    - `value`: raw count in the nominal `-2048..2047` range.
    - `width`: bar width in characters.
    Output:
    - An integer index clamped to the valid bar range `0..width-1`.
    """

    if width <= 1:
        return 0
    normalized = (value + 2048) / 4095.0
    index = int(round(normalized * (width - 1)))
    return max(0, min(width - 1, index))


def render_bar(current: int, minimum: int, maximum: int, width: int, connected: bool) -> str:
    """Render one ASCII bar showing current/min/max analogue values.

    Inputs:
    - `current`: latest raw sample count.
    - `minimum`: minimum count seen in the current report window.
    - `maximum`: maximum count seen in the current report window.
    - `width`: desired bar width in characters.
    - `connected`: whether the related jack is currently detected as plugged in.
    Output:
    - One ASCII bar string suitable for inclusion in the terminal dashboard.
    """

    fill = "." if connected else " "
    chars = [fill for _ in range(width)]
    zero_index = clamp_index(0, width)
    chars[zero_index] = "|"

    min_index = clamp_index(minimum, width)
    max_index = clamp_index(maximum, width)
    current_index = clamp_index(current, width)

    chars[min_index] = "<"
    chars[max_index] = ">"
    chars[current_index] = "*"

    if min_index == current_index == max_index:
        chars[current_index] = "#"
    elif current_index == min_index:
        chars[current_index] = "x"
    elif current_index == max_index:
        chars[current_index] = "+"

    return "".join(chars)


def format_connection(label: str, connected: int, color_enabled: bool) -> str:
    """Format one jack-detect flag as a colored on/off label.

    Inputs:
    - `label`: short channel label such as `A1` or `P2`.
    - `connected`: `1` when plugged, `0` when disconnected.
    - `color_enabled`: whether ANSI colors should be used.
    Output:
    - A compact connection-status string for the dashboard.
    """

    state_text = "ON " if connected else "OFF"
    color = COLOR_GREEN if connected else COLOR_DIM
    return colorize(f"{label}:{state_text}", color, color_enabled)


def render_analog_line(
    label: str,
    current: int,
    minimum: int,
    maximum: int,
    connected: int,
    color_enabled: bool,
) -> str:
    """Render one dashboard line for an analogue channel.

    Inputs:
    - `label`: display name such as `Audio 1`.
    - `current`: latest raw count.
    - `minimum`: minimum count from the current 50 ms window.
    - `maximum`: maximum count from the current 50 ms window.
    - `connected`: jack-detect flag for the channel.
    - `color_enabled`: whether ANSI color output is enabled.
    Output:
    - One fully formatted dashboard line.
    """

    status = colorize("connected", COLOR_GREEN, color_enabled) if connected else colorize(
        "disconnected", COLOR_DIM, color_enabled
    )
    bar = render_bar(current, minimum, maximum, 41, bool(connected))
    return (
        f"{label:<8} {status:<20} "
        f"cur={current:>5} min={minimum:>5} max={maximum:>5}  [{bar}]"
    )


def render_pulse_line(
    label: str,
    state: int,
    edges: int,
    connected: int,
    color_enabled: bool,
) -> str:
    """Render one dashboard line for a pulse input.

    Inputs:
    - `label`: display name such as `Pulse 1`.
    - `state`: current high/low state from the firmware stream.
    - `edges`: rising-edge count during the current 50 ms window.
    - `connected`: jack-detect flag for the pulse input.
    - `color_enabled`: whether ANSI color output is enabled.
    Output:
    - One fully formatted dashboard line.
    """

    state_text = colorize("HIGH", COLOR_YELLOW, color_enabled) if state else colorize(
        "LOW ", COLOR_DIM, color_enabled
    )
    status = colorize("connected", COLOR_GREEN, color_enabled) if connected else colorize(
        "disconnected", COLOR_DIM, color_enabled
    )
    meter = "#" * min(edges, 20)
    return (
        f"{label:<8} {status:<20} "
        f"state={state_text} edges={edges:>4}  [{meter:<20}]"
    )


def update_peak_hold(
    hold: PeakHold | None,
    reported_minimum: int,
    reported_maximum: int,
    now: float,
    hold_seconds: float,
) -> PeakHold:
    """Update one analogue peak-hold pair from the latest reported min/max values.

    Inputs:
    - `hold`: previous hold state for one analogue channel, or `None` on first frame.
    - `reported_minimum`: latest device-window minimum.
    - `reported_maximum`: latest device-window maximum.
    - `now`: current monotonic timestamp.
    - `hold_seconds`: host-side peak hold duration.
    Output:
    - Updated hold state for the channel.
    """

    if hold is None:
        return PeakHold(
            minimum=reported_minimum,
            maximum=reported_maximum,
            min_expires_at=now + hold_seconds,
            max_expires_at=now + hold_seconds,
        )

    if reported_minimum <= hold.minimum or now >= hold.min_expires_at:
        hold.minimum = reported_minimum
        hold.min_expires_at = now + hold_seconds

    if reported_maximum >= hold.maximum or now >= hold.max_expires_at:
        hold.maximum = reported_maximum
        hold.max_expires_at = now + hold_seconds

    return hold


def build_held_snapshot(
    snapshot: Snapshot,
    holds: dict[str, PeakHold],
    now: float,
    hold_seconds: float,
) -> HeldSnapshot:
    """Merge one firmware snapshot with host-side peak-hold state.

    Inputs:
    - `snapshot`: latest firmware report.
    - `holds`: mutable per-channel peak-hold map.
    - `now`: current monotonic timestamp.
    - `hold_seconds`: host-side peak hold duration.
    Output:
    - A `HeldSnapshot` ready for rendering.
    """

    holds["audio1"] = update_peak_hold(holds.get("audio1"), snapshot.audio1_min, snapshot.audio1_max, now, hold_seconds)
    holds["audio2"] = update_peak_hold(holds.get("audio2"), snapshot.audio2_min, snapshot.audio2_max, now, hold_seconds)
    holds["cv1"] = update_peak_hold(holds.get("cv1"), snapshot.cv1_min, snapshot.cv1_max, now, hold_seconds)
    holds["cv2"] = update_peak_hold(holds.get("cv2"), snapshot.cv2_min, snapshot.cv2_max, now, hold_seconds)

    return HeldSnapshot(
        sequence=snapshot.sequence,
        audio1=snapshot.audio1,
        audio1_min=holds["audio1"].minimum,
        audio1_max=holds["audio1"].maximum,
        audio2=snapshot.audio2,
        audio2_min=holds["audio2"].minimum,
        audio2_max=holds["audio2"].maximum,
        cv1=snapshot.cv1,
        cv1_min=holds["cv1"].minimum,
        cv1_max=holds["cv1"].maximum,
        cv2=snapshot.cv2,
        cv2_min=holds["cv2"].minimum,
        cv2_max=holds["cv2"].maximum,
        pulse1=snapshot.pulse1,
        pulse1_edges=snapshot.pulse1_edges,
        pulse2=snapshot.pulse2,
        pulse2_edges=snapshot.pulse2_edges,
        connected_audio1=snapshot.connected_audio1,
        connected_audio2=snapshot.connected_audio2,
        connected_cv1=snapshot.connected_cv1,
        connected_cv2=snapshot.connected_cv2,
        connected_pulse1=snapshot.connected_pulse1,
        connected_pulse2=snapshot.connected_pulse2,
    )


def render_dashboard(snapshot: HeldSnapshot, port: str, color_enabled: bool, hold_seconds: float) -> str:
    """Build the full terminal dashboard for one parsed snapshot.

    Inputs:
    - `snapshot`: most recent parsed input-monitor frame.
    - `port`: serial device currently being read.
    - `color_enabled`: whether ANSI colors should be included.
    - `hold_seconds`: current analogue min/max hold duration in seconds.
    Output:
    - A multi-line string that can be painted at the top of the terminal.
    """

    title = colorize("MTWS Input Monitor", COLOR_BOLD + COLOR_CYAN, color_enabled)
    timestamp = time.strftime("%H:%M:%S")
    connection_row = " ".join(
        [
            format_connection("A1", snapshot.connected_audio1, color_enabled),
            format_connection("A2", snapshot.connected_audio2, color_enabled),
            format_connection("CV1", snapshot.connected_cv1, color_enabled),
            format_connection("CV2", snapshot.connected_cv2, color_enabled),
            format_connection("P1", snapshot.connected_pulse1, color_enabled),
            format_connection("P2", snapshot.connected_pulse2, color_enabled),
        ]
    )

    lines = [
        f"{title}  port={port}  seq={snapshot.sequence}  updated={timestamp}",
        connection_row,
        "",
        render_analog_line(
            "Audio 1",
            snapshot.audio1,
            snapshot.audio1_min,
            snapshot.audio1_max,
            snapshot.connected_audio1,
            color_enabled,
        ),
        render_analog_line(
            "Audio 2",
            snapshot.audio2,
            snapshot.audio2_min,
            snapshot.audio2_max,
            snapshot.connected_audio2,
            color_enabled,
        ),
        render_analog_line(
            "CV 1",
            snapshot.cv1,
            snapshot.cv1_min,
            snapshot.cv1_max,
            snapshot.connected_cv1,
            color_enabled,
        ),
        render_analog_line(
            "CV 2",
            snapshot.cv2,
            snapshot.cv2_min,
            snapshot.cv2_max,
            snapshot.connected_cv2,
            color_enabled,
        ),
        "",
        render_pulse_line(
            "Pulse 1",
            snapshot.pulse1,
            snapshot.pulse1_edges,
            snapshot.connected_pulse1,
            color_enabled,
        ),
        render_pulse_line(
            "Pulse 2",
            snapshot.pulse2,
            snapshot.pulse2_edges,
            snapshot.connected_pulse2,
            color_enabled,
        ),
        "",
        colorize(
            f"Counts are raw ComputerCard values. Analogue min/max hold: {hold_seconds:.1f}s. Press Ctrl+C to exit.",
            COLOR_DIM,
            color_enabled,
        ),
    ]
    return "\n".join(lines)


def redraw_screen(content: str, color_enabled: bool) -> None:
    """Paint one dashboard frame at the top of the terminal.

    Inputs:
    - `content`: pre-rendered dashboard string.
    - `color_enabled`: whether ANSI control sequences should be emitted.
    Output:
    - The terminal is updated in place without scrolling.
    """

    if color_enabled:
        sys.stdout.write("\x1b[H\x1b[J")
    else:
        sys.stdout.write("\n" * 2)
    sys.stdout.write(content)
    sys.stdout.write("\n")
    sys.stdout.flush()


def open_serial_port(port: str, baud: int, timeout: float) -> serial.Serial:
    """Open the requested USB CDC serial device.

    Inputs:
    - `port`: serial device path.
    - `baud`: configured baud rate.
    - `timeout`: read timeout in seconds.
    Output:
    - An open `serial.Serial` object ready for line-oriented reads.
    """

    return serial.serial_for_url(url=port, baudrate=baud, timeout=timeout)


def read_snapshots(serial_port: serial.Serial) -> Iterable[Snapshot]:
    """Yield parsed snapshots from the USB serial stream.

    Inputs:
    - `serial_port`: an open `serial.Serial` object.
    Output:
    - An iterator of successfully parsed `Snapshot` objects.
    """

    while True:
        raw = serial_port.readline()
        if not raw:
            continue
        snapshot = parse_snapshot_line(raw.decode("utf-8", errors="replace"))
        if snapshot is not None:
            yield snapshot


def main() -> int:
    """Run the input-monitor terminal UI until interrupted.

    Inputs:
    - Command-line options for serial port, baud rate, and color mode.
    Output:
    - Process exit code `0` on normal termination, or non-zero on setup errors.
    """

    parser = build_argument_parser()
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    if not port:
        parser.error("Could not auto-detect a serial port. Re-run with --port /dev/...")

    color_enabled = not args.no_color and sys.stdout.isatty()

    try:
        with open_serial_port(port, args.baud, args.timeout) as serial_port:
            if color_enabled:
                sys.stdout.write("\x1b[2J\x1b[H\x1b[?25l")
                sys.stdout.flush()

            holds: dict[str, PeakHold] = {}
            for snapshot in read_snapshots(serial_port):
                now = time.monotonic()
                held_snapshot = build_held_snapshot(snapshot, holds, now, args.hold_seconds)
                redraw_screen(
                    render_dashboard(held_snapshot, port, color_enabled, args.hold_seconds),
                    color_enabled,
                )
    except serial.SerialException as exc:
        print(f"Failed to open or read serial port {port}: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 0
    finally:
        if color_enabled:
            sys.stdout.write("\x1b[?25h")
            sys.stdout.flush()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
