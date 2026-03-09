#!/usr/bin/env python3
"""Generate a browsable AKWF manifest with lightweight musical metadata."""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import time
import wave
from pathlib import Path


ANALYSIS_POINT_COUNT = 256
PREVIEW_POINT_COUNT = 64
HARMONIC_COUNT = 16


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments.

    Inputs: command-line flags from sys.argv.
    Outputs: argparse.Namespace containing source_root and output_path.
    """

    script_path = Path(__file__).resolve()
    default_source_root = script_path.with_name("AKWF")
    default_output_path = script_path.with_name("akwf_manifest.json")

    parser = argparse.ArgumentParser(
        description="Scan Adventure Kid single-cycle WAVs and build a JSON manifest.",
    )
    parser.add_argument(
        "--source-root",
        default=str(default_source_root),
        help="Absolute or relative path to the AKWF folder to scan.",
    )
    parser.add_argument(
        "--output",
        default=str(default_output_path),
        help="Destination JSON file path.",
    )
    return parser.parse_args()


def main() -> int:
    """Run the manifest generation workflow.

    Inputs: parsed command-line arguments.
    Outputs: process exit code. Returns 0 on success and 1 on fatal failure.
    """

    args = parse_args()
    source_root = Path(args.source_root).resolve()
    output_path = Path(args.output).resolve()

    if not source_root.exists():
        print(f"Source root does not exist: {source_root}", file=sys.stderr)
        return 1

    cosine_table, sine_table = build_dft_tables(ANALYSIS_POINT_COUNT, HARMONIC_COUNT)
    wave_paths = sorted(source_root.rglob("*.wav"))
    manifest_entries: list[dict[str, object]] = []
    category_counts: dict[str, int] = {}
    skipped_files: list[str] = []

    for wave_path in wave_paths:
        try:
            samples, sample_rate = read_wave_file(wave_path)
            entry = build_manifest_entry(
                wave_path=wave_path,
                source_root=source_root,
                samples=samples,
                sample_rate=sample_rate,
                cosine_table=cosine_table,
                sine_table=sine_table,
            )
            manifest_entries.append(entry)
            category_name = str(entry["category"])
            category_counts[category_name] = category_counts.get(category_name, 0) + 1
        except Exception as error:  # noqa: BLE001 - manifest build should continue after one bad file
            skipped_files.append(f"{wave_path}: {error}")

    categories = [
        {"name": category_name, "count": category_counts[category_name]}
        for category_name in sorted(category_counts)
    ]

    manifest = {
        "generated_at_epoch": int(time.time()),
        "source_root": str(source_root),
        "served_path_root": "./AKWF",
        "analysis_point_count": ANALYSIS_POINT_COUNT,
        "preview_point_count": PREVIEW_POINT_COUNT,
        "harmonic_count": HARMONIC_COUNT,
        "wave_count": len(manifest_entries),
        "skipped_count": len(skipped_files),
        "categories": categories,
        "waves": manifest_entries,
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as output_file:
        json.dump(manifest, output_file, indent=2)

    print(f"Wrote {len(manifest_entries)} entries to {output_path}")
    if skipped_files:
        print(f"Skipped {len(skipped_files)} files:", file=sys.stderr)
        for skipped_line in skipped_files[:20]:
            print(f"  {skipped_line}", file=sys.stderr)

    return 0


def build_dft_tables(point_count: int, harmonic_count: int) -> tuple[list[list[float]], list[list[float]]]:
    """Precompute cosine and sine tables for direct harmonic analysis.

    Inputs: point_count for the analysis cycle and harmonic_count for the number of bins.
    Outputs: tuple of [harmonic][sample] cosine and sine tables.
    """

    cosine_table: list[list[float]] = []
    sine_table: list[list[float]] = []

    for harmonic_index in range(1, harmonic_count + 1):
        cosine_row: list[float] = []
        sine_row: list[float] = []
        for sample_index in range(point_count):
            phase = (2.0 * math.pi * harmonic_index * sample_index) / point_count
            cosine_row.append(math.cos(phase))
            sine_row.append(math.sin(phase))
        cosine_table.append(cosine_row)
        sine_table.append(sine_row)

    return cosine_table, sine_table


def read_wave_file(wave_path: Path) -> tuple[list[float], int]:
    """Read a PCM WAV file and downmix it to mono floating-point samples.

    Inputs: wave_path pointing at one Adventure Kid WAV file.
    Outputs: tuple of (mono_samples, sample_rate) where mono_samples are in [-1, 1].
    """

    with wave.open(str(wave_path), "rb") as wave_file:
        sample_rate = wave_file.getframerate()
        channel_count = wave_file.getnchannels()
        sample_width = wave_file.getsampwidth()
        frame_count = wave_file.getnframes()
        raw_frames = wave_file.readframes(frame_count)

    if channel_count <= 0:
        raise ValueError("channel count must be positive")

    if sample_width == 1:
        frame_values = [(sample - 128) / 128.0 for sample in raw_frames]
    elif sample_width == 2:
        unpacked = struct.unpack("<" + "h" * (len(raw_frames) // 2), raw_frames)
        frame_values = [sample / 32768.0 for sample in unpacked]
    elif sample_width == 3:
        frame_values = unpack_pcm24(raw_frames)
    elif sample_width == 4:
        unpacked = struct.unpack("<" + "i" * (len(raw_frames) // 4), raw_frames)
        frame_values = [sample / 2147483648.0 for sample in unpacked]
    else:
        raise ValueError(f"unsupported sample width: {sample_width}")

    mono_samples: list[float] = []
    for frame_index in range(frame_count):
        frame_start = frame_index * channel_count
        mono_value = sum(frame_values[frame_start : frame_start + channel_count]) / channel_count
        mono_samples.append(max(-1.0, min(1.0, mono_value)))

    if not mono_samples:
        raise ValueError("file contains no audio frames")

    return mono_samples, sample_rate


def unpack_pcm24(raw_frames: bytes) -> list[float]:
    """Decode little-endian signed 24-bit PCM bytes into normalized floats.

    Inputs: raw_frames containing packed 24-bit PCM frame bytes.
    Outputs: list of sample values normalized into [-1, 1].
    """

    decoded: list[float] = []
    for byte_index in range(0, len(raw_frames), 3):
        sample = (
            raw_frames[byte_index]
            | (raw_frames[byte_index + 1] << 8)
            | (raw_frames[byte_index + 2] << 16)
        )
        if sample & 0x800000:
            sample -= 0x1000000
        decoded.append(sample / 8388608.0)
    return decoded


def build_manifest_entry(
    wave_path: Path,
    source_root: Path,
    samples: list[float],
    sample_rate: int,
    cosine_table: list[list[float]],
    sine_table: list[list[float]],
) -> dict[str, object]:
    """Compute metadata for one wave file and return a JSON-serializable entry.

    Inputs: file path information, decoded samples, sample rate, and harmonic lookup tables.
    Outputs: dictionary describing file location, waveform metrics, and harmonic summaries.
    """

    relative_path = wave_path.relative_to(source_root)
    category_name = relative_path.parts[0] if len(relative_path.parts) > 1 else "root"
    centered_samples = remove_dc_offset(samples)
    peak_abs = max(abs(sample) for sample in samples)
    rms_value = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
    zero_crossings = count_zero_crossings(centered_samples)

    normalized_analysis_cycle = normalize_peak(
        periodic_resample(centered_samples, ANALYSIS_POINT_COUNT),
    )
    preview_cycle = normalize_peak(
        periodic_resample(centered_samples, PREVIEW_POINT_COUNT),
    )

    harmonic_profile = compute_harmonic_profile(
        normalized_analysis_cycle,
        cosine_table,
        sine_table,
    )
    harmonic_total = sum(harmonic_profile)
    spectral_centroid = (
        sum((harmonic_index + 1) * harmonic_profile[harmonic_index] for harmonic_index in range(HARMONIC_COUNT))
        / harmonic_total
        if harmonic_total > 0.0
        else 0.0
    )
    odd_harmonic_total = sum(harmonic_profile[0::2])
    high_harmonic_total = sum(harmonic_profile[8:])
    dominant_harmonic = (
        max(range(HARMONIC_COUNT), key=lambda harmonic_index: harmonic_profile[harmonic_index]) + 1
        if harmonic_total > 0.0
        else 0
    )

    return {
        "id": relative_path.as_posix(),
        "category": category_name,
        "file_name": wave_path.name,
        "display_name": wave_path.stem,
        "relative_path": f"./AKWF/{relative_path.as_posix()}",
        "sample_rate": sample_rate,
        "sample_count": len(samples),
        "duration_samples": len(samples),
        "dc_offset": round(sum(samples) / len(samples), 6),
        "peak_abs": round(peak_abs, 6),
        "rms": round(rms_value, 6),
        "zero_crossings": zero_crossings,
        "spectral_centroid": round(spectral_centroid, 6),
        "brightness_score": round(max(0.0, min(1.0, (spectral_centroid - 1.0) / (HARMONIC_COUNT - 1))), 6),
        "high_harmonic_ratio": round(high_harmonic_total / harmonic_total, 6) if harmonic_total > 0.0 else 0.0,
        "odd_harmonic_ratio": round(odd_harmonic_total / harmonic_total, 6) if harmonic_total > 0.0 else 0.0,
        "dominant_harmonic": dominant_harmonic,
        "harmonic_profile_16": [round(value, 6) for value in harmonic_profile],
        "preview_points_q7": [quantize_q7(value) for value in preview_cycle],
    }


def remove_dc_offset(samples: list[float]) -> list[float]:
    """Subtract the mean from a waveform so shape metrics ignore DC bias.

    Inputs: samples from one mono waveform.
    Outputs: new list of samples centered around zero.
    """

    mean_value = sum(samples) / len(samples)
    return [sample - mean_value for sample in samples]


def normalize_peak(samples: list[float]) -> list[float]:
    """Scale a waveform so its largest absolute sample reaches unity.

    Inputs: samples from one waveform, usually already DC-corrected.
    Outputs: new list of peak-normalized samples in approximately [-1, 1].
    """

    peak_value = max(abs(sample) for sample in samples)
    if peak_value <= 0.0:
        return [0.0 for _ in samples]
    return [sample / peak_value for sample in samples]


def periodic_resample(samples: list[float], point_count: int) -> list[float]:
    """Resample one periodic cycle with wraparound interpolation.

    Inputs: source samples for one cycle and the requested output point count.
    Outputs: point_count evenly spaced samples describing the same periodic cycle.
    """

    source_count = len(samples)
    if source_count == point_count:
        return list(samples)

    resampled: list[float] = []
    for point_index in range(point_count):
        source_position = (point_index * source_count) / point_count
        source_index = int(source_position) % source_count
        source_fraction = source_position - math.floor(source_position)
        next_index = (source_index + 1) % source_count
        sample_a = samples[source_index]
        sample_b = samples[next_index]
        resampled.append(sample_a + (sample_b - sample_a) * source_fraction)
    return resampled


def count_zero_crossings(samples: list[float]) -> int:
    """Count sign changes in a centered waveform.

    Inputs: DC-corrected waveform samples.
    Outputs: integer number of sign transitions across the full periodic cycle.
    """

    signs = [1 if sample > 0.0 else -1 if sample < 0.0 else 0 for sample in samples]
    non_zero_signs = [sign for sign in signs if sign != 0]

    if not non_zero_signs:
        return 0

    crossing_count = 0
    for sign_index in range(len(non_zero_signs)):
        current_sign = non_zero_signs[sign_index]
        next_sign = non_zero_signs[(sign_index + 1) % len(non_zero_signs)]
        if current_sign != next_sign:
            crossing_count += 1

    return crossing_count


def compute_harmonic_profile(
    samples: list[float],
    cosine_table: list[list[float]],
    sine_table: list[list[float]],
) -> list[float]:
    """Measure the first harmonic_count magnitudes of one normalized waveform.

    Inputs: peak-normalized cycle samples and the precomputed DFT tables.
    Outputs: normalized harmonic magnitudes whose sum is 1.0 when energy exists.
    """

    magnitudes: list[float] = []

    for harmonic_index in range(HARMONIC_COUNT):
        real_sum = 0.0
        imag_sum = 0.0
        for sample_index, sample in enumerate(samples):
            real_sum += sample * cosine_table[harmonic_index][sample_index]
            imag_sum -= sample * sine_table[harmonic_index][sample_index]
        magnitudes.append(math.sqrt(real_sum * real_sum + imag_sum * imag_sum))

    magnitude_total = sum(magnitudes)
    if magnitude_total <= 0.0:
        return [0.0 for _ in magnitudes]

    return [magnitude / magnitude_total for magnitude in magnitudes]


def quantize_q7(value: float) -> int:
    """Convert a floating-point sample into a signed 7-bit-ish preview integer.

    Inputs: floating-point sample expected in [-1, 1].
    Outputs: integer in [-127, 127] suitable for compact JSON preview storage.
    """

    return max(-127, min(127, int(round(value * 127.0))))


if __name__ == "__main__":
    raise SystemExit(main())
