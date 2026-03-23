#!/bin/zsh

set -euo pipefail

SCRIPT_DIR="${0:A:h}"
VENV_JUPYTER="$SCRIPT_DIR/.venv/bin/jupyter"
NOTEBOOK="$SCRIPT_DIR/phase_distortion_visuals.ipynb"
export MPLCONFIGDIR="$SCRIPT_DIR/.mplconfig"

mkdir -p "$MPLCONFIGDIR"

exec "$VENV_JUPYTER" notebook "$NOTEBOOK"
