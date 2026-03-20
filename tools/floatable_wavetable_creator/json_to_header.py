#!/usr/bin/env python3
"""Convert a floatable selection JSON export into a 16 x 256 C header."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

TARGET_SLOT_COUNT = 16
CYCLE_POINT_COUNT = 256
INT16_MIN = -32768
INT16_MAX = 32767


def parse_arguments() -> argparse.Namespace:
    """Parse command-line arguments for the JSON-to-header converter.

    Inputs: none.
    Outputs: argparse namespace containing the input path, output path, and optional symbol name.
    """

    parser = argparse.ArgumentParser(
        description="Convert a floatable selection JSON export into a 16 x 256 int16_t C header.",
    )
    parser.add_argument(
        "input_json",
        type=Path,
        help="Path to the exported floatable selection JSON file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Optional output header path. Defaults to the input path with a .h suffix.",
    )
    parser.add_argument(
        "-s",
        "--symbol",
        default=None,
        help="Optional C symbol name for the generated wavetable array.",
    )
    return parser.parse_args()


def load_selection_json(input_path: Path) -> dict[str, Any]:
    """Load and parse one exported selection JSON file from disk.

    Inputs: input_path - filesystem path to the JSON export produced by the wavetable browser.
    Outputs: parsed top-level JSON object.
    """

    try:
        file_text = input_path.read_text(encoding="utf-8")
    except OSError as error:
        raise ValueError(f"Could not read {input_path}: {error}") from error

    try:
        payload = json.loads(file_text)
    except json.JSONDecodeError as error:
        raise ValueError(f"Could not parse JSON in {input_path}: {error}") from error

    if not isinstance(payload, dict):
        raise ValueError("Selection JSON must contain one top-level object.")

    return payload


def extract_slot_cycles(selection_payload: dict[str, Any]) -> list[list[int]]:
    """Validate the exported selection payload and extract its 16 wave arrays.

    Inputs: selection_payload - parsed JSON object from the browser export.
    Outputs: list of 16 wave arrays, each containing 256 signed 16-bit integers.
    """

    slot_count = selection_payload.get("slot_count")
    point_count = selection_payload.get("point_count")
    slots = selection_payload.get("slots")

    if slot_count != TARGET_SLOT_COUNT:
        raise ValueError(f"Expected slot_count {TARGET_SLOT_COUNT}, received {slot_count}.")

    if point_count != CYCLE_POINT_COUNT:
        raise ValueError(f"Expected point_count {CYCLE_POINT_COUNT}, received {point_count}.")

    if not isinstance(slots, list) or len(slots) != TARGET_SLOT_COUNT:
        raise ValueError(f"Selection JSON must contain exactly {TARGET_SLOT_COUNT} slot entries.")

    slot_cycles: list[list[int]] = []
    for slot_index, slot_payload in enumerate(slots):
        slot_cycles.append(validate_slot_payload(slot_payload, slot_index))

    return slot_cycles


def validate_slot_payload(slot_payload: Any, slot_index: int) -> list[int]:
    """Validate one slot object and return its Q15 cycle points.

    Inputs: slot_payload - one slot entry from the exported JSON file, slot_index - zero-based slot index.
    Outputs: one 256-point signed integer waveform array.
    """

    if not isinstance(slot_payload, dict):
        raise ValueError(f"Slot {slot_index + 1:02d} is not a valid object.")

    declared_slot_index = slot_payload.get("slot_index")
    if declared_slot_index != slot_index:
        raise ValueError(
            f"Slot {slot_index + 1:02d} expected slot_index {slot_index}, received {declared_slot_index}.",
        )

    cycle_points = slot_payload.get("cycle_points_q15_256")
    if not isinstance(cycle_points, list) or len(cycle_points) != CYCLE_POINT_COUNT:
        raise ValueError(f"Slot {slot_index + 1:02d} must contain {CYCLE_POINT_COUNT} cycle points.")

    validated_points: list[int] = []
    for point_index, point_value in enumerate(cycle_points):
        if not isinstance(point_value, int) or not INT16_MIN <= point_value <= INT16_MAX:
            raise ValueError(
                f"Slot {slot_index + 1:02d} point {point_index} is not a signed 16-bit integer.",
            )
        validated_points.append(point_value)

    return validated_points


def derive_output_path(input_path: Path, requested_output: Path | None) -> Path:
    """Choose the output header path for one conversion run.

    Inputs: input_path - source JSON path, requested_output - optional explicit header path from the CLI.
    Outputs: filesystem path where the generated header should be written.
    """

    if requested_output is not None:
        return requested_output

    return input_path.with_suffix(".h")


def derive_symbol_name(input_path: Path, requested_symbol: str | None) -> str:
    """Choose a valid C identifier for the generated wavetable symbol.

    Inputs: input_path - source JSON path, requested_symbol - optional explicit symbol name from the CLI.
    Outputs: sanitized C identifier string used in the generated header.
    """

    raw_symbol = requested_symbol if requested_symbol else input_path.stem
    sanitized_symbol = re.sub(r"[^0-9A-Za-z_]+", "_", raw_symbol).strip("_")
    if not sanitized_symbol:
        sanitized_symbol = "floatable_selection_16x256"

    if sanitized_symbol[0].isdigit():
        sanitized_symbol = f"selection_{sanitized_symbol}"

    return sanitized_symbol


def build_include_guard(output_path: Path, symbol_name: str) -> str:
    """Build one conventional include guard token for the generated header.

    Inputs: output_path - destination header path, symbol_name - generated array symbol.
    Outputs: uppercase include guard token string.
    """

    raw_guard = f"{output_path.stem}_{symbol_name}_h"
    sanitized_guard = re.sub(r"[^0-9A-Za-z_]+", "_", raw_guard).upper().strip("_")
    if not sanitized_guard:
        sanitized_guard = "FLOATABLE_SELECTION_16X256_H"
    return sanitized_guard


def format_wave_rows(slot_cycles: list[list[int]]) -> str:
    """Format the 16 wave arrays into readable C initializer text.

    Inputs: slot_cycles - validated 16 x 256 signed integer waveform data.
    Outputs: multiline C initializer body ready to place inside the header.
    """

    formatted_slots: list[str] = []
    for slot_index, cycle_points in enumerate(slot_cycles):
        formatted_lines = [f"  // Slot {slot_index + 1:02d}"]
        formatted_lines.append("  {")

        for point_offset in range(0, len(cycle_points), 8):
            row_slice = cycle_points[point_offset : point_offset + 8]
            row_text = ", ".join(f"{point_value:6d}" for point_value in row_slice)
            trailing_comma = "," if point_offset + 8 < len(cycle_points) else ""
            formatted_lines.append(f"    {row_text}{trailing_comma}")

        formatted_lines.append("  }")
        formatted_slots.append("\n".join(formatted_lines))

    return ",\n".join(formatted_slots)


def render_header_text(
    input_path: Path,
    output_path: Path,
    symbol_name: str,
    slot_cycles: list[list[int]],
    exported_epoch: Any,
) -> str:
    """Render the final header source text for one converted selection file.

    Inputs: input_path - source JSON path, output_path - destination header path,
    symbol_name - generated array symbol, slot_cycles - validated waveform data,
    exported_epoch - optional export timestamp copied from the source JSON file.
    Outputs: complete header text ready to write to disk.
    """

    include_guard = build_include_guard(output_path, symbol_name)
    export_comment = (
        f"// Exported epoch: {exported_epoch}\n" if isinstance(exported_epoch, int) else ""
    )

    return (
        f"// Generated by json_to_header.py from {input_path.name}.\n"
        f"{export_comment}"
        f"// Shape: [{TARGET_SLOT_COUNT}][{CYCLE_POINT_COUNT}] signed 16-bit single-cycle waves.\n"
        "#ifndef "
        f"{include_guard}\n"
        "#define "
        f"{include_guard}\n\n"
        "#include <stdint.h>\n\n"
        f"static const int16_t {symbol_name}[{TARGET_SLOT_COUNT}][{CYCLE_POINT_COUNT}] = {{\n"
        f"{format_wave_rows(slot_cycles)}\n"
        "};\n\n"
        "#endif  // "
        f"{include_guard}\n"
    )


def write_header(output_path: Path, header_text: str) -> None:
    """Write the generated header text to disk.

    Inputs: output_path - filesystem path where the header should be stored, header_text - full header contents.
    Outputs: none. The destination file is created or overwritten.
    """

    try:
        output_path.write_text(header_text, encoding="utf-8")
    except OSError as error:
        raise ValueError(f"Could not write {output_path}: {error}") from error


def main() -> int:
    """Run the converter from the command line.

    Inputs: none.
    Outputs: process exit code, where 0 indicates success and 1 indicates validation or I/O failure.
    """

    args = parse_arguments()
    input_path = args.input_json.resolve()
    output_path = derive_output_path(input_path, args.output.resolve() if args.output else None)
    symbol_name = derive_symbol_name(input_path, args.symbol)

    try:
        selection_payload = load_selection_json(input_path)
        slot_cycles = extract_slot_cycles(selection_payload)
        header_text = render_header_text(
            input_path,
            output_path,
            symbol_name,
            slot_cycles,
            selection_payload.get("exported_at_epoch"),
        )
        write_header(output_path, header_text)
    except ValueError as error:
        print(str(error), file=sys.stderr)
        return 1

    print(f"Wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
