#!/bin/bash

usage() {
  echo "Usage: $0 <PWM_PERCENT> [FAN_TRAY] "
  echo "<PWM_PERCENT> : From 0 to 100 "
  echo "[FAN_TRAY] : 1, 2, 3, 4, 5 or 6."
  echo "If no FAN_TRAY is specified, all FANs will be programmed"
}

FANCPLD="/run/devmap/sensors/FAN_CPLD"
SLGCONTROLLER="/run/devmap/sensors/FS_FAN_SLG4F4527"

set -e

# Make sure we only have 1 or 2 arguments
if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# Also make sure that percent value is really between 0 and 100
if [ "$1" -lt 0 ] || [ "$1" -gt 100 ]; then
    echo "PWM_PERCENT $1: should be between 0 and 100"
    exit 1
fi

# Read FAN_TRAY value from user argument.
# If no FAN_TRAY is given, configure all FANs
if [ "$#" -eq 1 ]; then
    FAN_UNIT="1 2 3 4 5 6"
else
    if [ "$2" -le 0 ] || [ "$2" -ge 7 ]; then
        echo "Fan $2: should be between 1 and 6"
        exit 1
    fi
    FAN_UNIT="$2"
fi

# Convert input (0-100%) to PWM (0-255)
# Multiple by 2.5
step="$(printf '%.*f' 0 "$((255 * $1 / 100))")"

for unit in ${FAN_UNIT}; do
    if [ "$unit" -le 5 ]; then
        pwm="$FANCPLD/pwm${unit}"
        echo "$step" > "${pwm}"
        echo "$step will be set to ${pwm} !!!!"
        echo "Successfully set system fan ${unit} speed to $1%"
    else
        pwm="$SLGCONTROLLER/pwm"
        # Need to invert PWM setting for SLG because written value is lower duty cycle
        step="$(printf '%.*f' 0 "$((255 * (100 - $1) / 100))")"
        echo "$step" > "${pwm}"
        echo "$step will be set to ${pwm} !!!!"
        echo "Successfully set fanspinner fan speed to $1%"
    fi
done
