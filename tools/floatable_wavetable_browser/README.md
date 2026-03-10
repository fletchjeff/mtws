# Floatable AKWF Curator

This tool is now a library browser and curation surface for `todo #3`.

It is self-contained inside this folder, including the local Adventure Kid
collection at:

- `/Users/jeff/Toonbox/MTWS/mtws/tools/floatable_wavetable_browser/AKWF`

The workflow is:

- generate a manifest from the AKWF folder
- browse and filter the wave library in the browser
- inspect one candidate wave at a time
- place waves into `16` slots
- reorder the slot bank before committing
- audition one candidate, one slot, or a slow morph across the filled slots
- export the current bank as JSON or a concatenated bank WAV
- load a previously exported selection JSON back into the slot board

## Manifest generation

Run this from the tool folder or by using the absolute path:

```sh
cd /Users/jeff/Toonbox/MTWS/mtws/tools/floatable_wavetable_browser
python3 generate_akwf_manifest.py
```

This writes:

- `/Users/jeff/Toonbox/MTWS/mtws/tools/floatable_wavetable_browser/akwf_manifest.json`

## What the manifest stores

Each wave entry includes:

- source path and category
- sample count and sample rate
- `dc_offset`
- `peak_abs`
- `rms`
- `zero_crossings`
- `dominant_harmonic`
- `harmonic_profile_16`
- `spectral_centroid`
- `brightness_score`
- `high_harmonic_ratio`
- `odd_harmonic_ratio`
- `preview_points_q7`

The metadata is intentionally simple and musical:

- `zero_crossings`: wrapped sign changes across the single cycle
- `brightness_score`: normalized spectral centroid over the first `16` harmonics
- `high_harmonic_ratio`: harmonic energy in bins `9..16` versus total
- `odd_harmonic_ratio`: odd-harmonic energy versus total harmonic energy

## Run locally

Serve directly from this folder:

```sh
cd /Users/jeff/Toonbox/MTWS/mtws/tools/floatable_wavetable_browser
python3 -m http.server 8765
```

Then open:

```text
http://127.0.0.1:8765/
```

## Demo mode

For a quick UI smoke test, this query fills the `16` slots with the first `16`
visible library entries:

```text
http://127.0.0.1:8765/?demo=1
```

## Export behavior

- `Export Selection JSON`: writes the current `16` slots plus `256`-point signed
  cycle data for each slot
- `Load Selection JSON`: reads one previously exported selection file from your
  local machine and restores the `16` slots, with validation and clear errors
  when the file shape is wrong
- `Export Bank WAV`: writes a concatenated `16 x 256` mono WAV bank

## Convert JSON to a C header

Use the standalone Python converter when you want a firmware-style header from
an exported selection JSON file:

```sh
cd /Users/jeff/Toonbox/MTWS/mtws/tools/floatable_wavetable_browser
python3 json_to_header.py floatable_selection_16.json
```

That writes a sibling header by default, for example:

```text
floatable_selection_16.h
```

You can also choose a destination path and C symbol explicitly:

```sh
python3 json_to_header.py floatable_selection_16.json \
  --output /tmp/my_bank.h \
  --symbol my_bank_16x256
```

The converter validates the exported shape before writing:

- `slot_count == 16`
- `point_count == 256`
- `16` slot entries
- `256` signed `int16_t` samples in every slot
