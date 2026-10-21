#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_dir=$(dirname -- "$script_dir")
test_binary=$(mktemp /tmp/cyd-weather-test.XXXXXX)
trap 'rm -f -- "$test_binary"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$project_dir/firmware/include" \
    "$project_dir/firmware/tests/weather_store_test.cpp" -o "$test_binary"
"$test_binary"
g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$project_dir/firmware/include" \
    "$project_dir/firmware/tests/update_policy_test.cpp" -o "$test_binary"
"$test_binary"
g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$project_dir/firmware/include" \
    "$project_dir/firmware/tests/sensor_health_test.cpp" -o "$test_binary"
"$test_binary"
g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$project_dir/firmware/include" \
    "$project_dir/firmware/tests/setup_credentials_test.cpp" -o "$test_binary"
"$test_binary"
g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
    -I"$project_dir/firmware/include" \
    "$project_dir/firmware/tests/reporting_test.cpp" -o "$test_binary"
"$test_binary"
json_headers="$project_dir/firmware/.pio/libdeps/panellan_sc05_x/ArduinoJson/src"
if [[ -d "$json_headers" ]]; then
  g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
      -I"$project_dir/firmware/include" -I"$json_headers" \
      "$project_dir/firmware/tests/atlas_packet_test.cpp" -o "$test_binary"
  if [[ -f "$project_dir/captures/atlas/packets-2026-10-18.jsonl" ]]; then
    "$test_binary" "$project_dir/captures/atlas/packets-2026-10-18.jsonl"
  else
    "$test_binary"
  fi
else
  echo 'Atlas JSON adapter check needs the pinned build dependencies: run bash tools/pio.sh run -d firmware first.'
fi
tool_env="${XDG_DATA_HOME:-$HOME/.local/share}/cyd-weather-link/venv"
"$tool_env/bin/python" -m unittest discover -s "$script_dir" -p 'test_firmware_console.py' -v
