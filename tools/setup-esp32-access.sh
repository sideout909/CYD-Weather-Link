#!/usr/bin/env bash
# Install the serial permission rule for the currently owned PanelLan board.
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
    echo 'Run this script with sudo from your terminal.' >&2
    exit 1
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
install -o root -g root -m 0644 -- \
    "$script_dir/70-cyd-weather-link.rules" \
    /etc/udev/rules.d/70-cyd-weather-link.rules
udevadm control --reload-rules

device_link=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_68:B6:B3:34:82:48-if00
if [[ -e "$device_link" ]]; then
    device=$(readlink -f -- "$device_link")
    device_path=$(udevadm info --query=path --name="$device")
    udevadm trigger --action=add "/sys$device_path"
    udevadm settle
    # Apply access immediately to this account as well as the active-seat rule.
    setfacl -m u:lschaar:rw -- "$device"
    ls -l -- "$device"
else
    echo 'Rule installed. Connect the PanelLan board to apply it.'
fi
