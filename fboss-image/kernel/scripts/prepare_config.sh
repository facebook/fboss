#!/bin/bash
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
# prepare_config.sh - Create kernel config by merging FBOSS reference config with kernel defaults
#
# Usage: prepare_config.sh <kernel_source_path> <fboss_reference_config>
#
# This script:
# 1. Creates defconfig in the kernel source directory
# 2. Merges FBOSS reference config settings into the base config
#    and applies optional local overrides
# 3. Reconciles defaults via 'make olddefconfig'
#
set -euo pipefail
PROGNAME=$(basename "$0")

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  echo "Usage: $PROGNAME <kernel_source_path> <fboss_reference_config> [local_overrides_config]"
  echo ""
  echo "Create kernel config by merging FBOSS reference config with kernel defaults"
  echo ""
  echo "Arguments:"
  echo "  kernel_source_path     Path to Linux kernel source directory"
  echo "  fboss_reference_config Path to FBOSS reference kernel config file"
  echo "  local_overrides_config Optional path to local config overrides file"
  echo ""
  echo "Examples:"
  echo "  $PROGNAME /path/to/linux-6.4.3 fboss-reference.config"
  echo "  $PROGNAME /path/to/linux-6.4.3 fboss-reference.config fboss-local-overrides.config"
}

validate_inputs() {
  local kernel_source="$1"
  local fboss_config="$2"

  if [ ! -d "$kernel_source" ]; then
    echo "$PROGNAME: Error: Kernel source directory '$kernel_source' does not exist"
    exit 1
  fi

  if [ ! -f "$fboss_config" ]; then
    echo "$PROGNAME: Error: FBOSS reference config '$fboss_config' does not exist"
    exit 2
  fi
}

# Main script logic
if [ $# -lt 2 ] || [ $# -gt 3 ]; then
  usage
  exit 3
fi

KERNEL_SOURCE="$1"
FBOSS_CONFIG="$2"
LOCAL_OVERRIDES="${3:-}"

validate_inputs "$KERNEL_SOURCE" "$FBOSS_CONFIG"

echo "Preparing kernel config..."
echo "Kernel source: $KERNEL_SOURCE"
echo "FBOSS config: $FBOSS_CONFIG"
if [[ -n $LOCAL_OVERRIDES ]]; then
  echo "Local overrides: $LOCAL_OVERRIDES"
fi

# Create default config
echo "Creating defconfig"
cd "$KERNEL_SOURCE"

make defconfig

# Merge FBOSS reference config into the default config
python3 "$SCRIPTS_DIR/merge_config.py" ".config" "$FBOSS_CONFIG"

# Apply local overrides (third layer)
if [[ -n $LOCAL_OVERRIDES ]]; then
  python3 "$SCRIPTS_DIR/merge_config.py" ".config" "$LOCAL_OVERRIDES"
fi

# Reconcile any dependency-driven defaults after all merges
make olddefconfig

echo "Kernel config preparation complete!"
