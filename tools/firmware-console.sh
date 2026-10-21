#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
tool_env="${XDG_DATA_HOME:-$HOME/.local/share}/cyd-weather-link/venv"
exec "$tool_env/bin/python" "$script_dir/firmware_console.py" "$@"
