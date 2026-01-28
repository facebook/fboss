#!/usr/bin/env bash
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

# Test the real FBOSS reference + local overrides produce the expected .config
# - Asserts to ensure that FBOSS ref config options remain
# - Asserts to ensure that overrides replace existing values (no duplicates)
#   and result is exactly as requested
# - Uses a fake kernel tree (defconfig/olddefconfig stubs) for quick sanity check

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../env/kernel_build_env.sh"

TEST_DIR="$(mktemp -d -p "$SCRIPT_DIR" tmp_prepare_config_real.XXXXXX)"

FAKE_KERNEL_DIR="$TEST_DIR/fake_kernel"
# Use canonical paths from the environment to avoid duplicate segments
REF_CONFIG="$KERNEL_CONFIGS_DIR/fboss-reference.config"
OVR_CONFIG="$TEST_DIR/fboss-local-overrides.config"

cleanup() {
  rm -rf "$TEST_DIR"
}
trap cleanup EXIT

# Generate the local overrides config from YAML for testing
# Use the first kernel version from FBOSS_KERNEL_VERSIONS array
TEST_KERNEL_VERSION="${FBOSS_KERNEL_VERSIONS[0]}"
python3 "$KERNEL_SCRIPTS_DIR/generate_config_overrides.py" \
  "$KERNEL_CONFIGS_DIR/fboss-local-overrides.yaml" \
  "$TEST_KERNEL_VERSION" \
  "$OVR_CONFIG"

mkdir -p "$FAKE_KERNEL_DIR"

# Minimal fake kernel Makefile providing defconfig and olddefconfig
cat >"$FAKE_KERNEL_DIR/Makefile" <<'EOF'
ARCH ?= x86_64

defconfig:
  @echo "# Fake kernel defconfig" > .config
  @echo "CONFIG_64BIT=y" >> .config
  @echo "CONFIG_X86_64=y" >> .config

olddefconfig:
  @echo "# Reconciled by olddefconfig" >> .config

.PHONY: defconfig olddefconfig
EOF

# Run with real reference + overrides
cd "$FAKE_KERNEL_DIR"
"$KERNEL_CONFIG_SCRIPT" "$FAKE_KERNEL_DIR" "$REF_CONFIG" "$OVR_CONFIG"

# Helper: assert a grep matches
assert_grep() {
  local pattern="$1"
  local file="$2"
  local msg="$3"
  if ! grep -qE "$pattern" "$file"; then
    echo "FAIL: $msg" >&2
    echo "Searched pattern: $pattern" >&2
    exit 1
  else
    echo "PASS: $msg"
  fi
}

# Helper: assert a grep does NOT match
assert_not_grep() {
  local pattern="$1"
  local file="$2"
  local msg="$3"
  if grep -qE "$pattern" "$file"; then
    echo "FAIL: $msg" >&2
    echo "Unexpectedly matched pattern: $pattern" >&2
    exit 1
  else
    echo "PASS: $msg"
  fi
}

CFG=".config"

# 1) FBOSS reference presence (pick stable, known entries)
assert_grep '^CONFIG_NET_NS=y$' "$CFG" "FBOSS ref: NET_NS present"
assert_grep '^CONFIG_BRIDGE=m$' "$CFG" "FBOSS ref: BRIDGE=m present"
assert_grep '^CONFIG_VLAN_8021Q=m$' "$CFG" "FBOSS ref: VLAN_8021Q=m present"

# 2) Overrides are applied exactly (set to empty string) and no duplicates remain
# CONFIG_BOOT_CONFIG_EMBED_FILE
assert_grep '^CONFIG_BOOT_CONFIG_EMBED_FILE=""$' "$CFG" "Override: BOOT_CONFIG_EMBED_FILE set to empty"
assert_not_grep '^# CONFIG_BOOT_CONFIG_EMBED_FILE is not set$' "$CFG" "Override: BOOT_CONFIG_EMBED_FILE not left as not-set"
assert_not_grep 'facebook/config/common\.bootconfig' "$CFG" "Override: No FB path remains for BOOT_CONFIG_EMBED_FILE"

# Ensure no duplicate definitions for this symbol
if [[ $(grep -E '^(# )?CONFIG_BOOT_CONFIG_EMBED_FILE' "$CFG" | wc -l) -ne 1 ]]; then
  echo "FAIL: BOOT_CONFIG_EMBED_FILE appears more than once" >&2
  grep -nE '^(# )?CONFIG_BOOT_CONFIG_EMBED_FILE' "$CFG" || true
  exit 1
else
  echo "PASS: BOOT_CONFIG_EMBED_FILE appears exactly once"
fi

# CONFIG_SYSTEM_TRUSTED_KEYS
assert_grep '^CONFIG_SYSTEM_TRUSTED_KEYS=""$' "$CFG" "Override: SYSTEM_TRUSTED_KEYS set to empty"
assert_not_grep '^# CONFIG_SYSTEM_TRUSTED_KEYS is not set$' "$CFG" "Override: SYSTEM_TRUSTED_KEYS not left as not-set"
assert_not_grep 'facebook/fbinfra-kmod-hsm\.pem' "$CFG" "Override: No FB path remains for SYSTEM_TRUSTED_KEYS"
# Ensure no duplicate definitions for this symbol
if [[ $(grep -E '^(# )?CONFIG_SYSTEM_TRUSTED_KEYS' "$CFG" | wc -l) -ne 1 ]]; then
  echo "FAIL: SYSTEM_TRUSTED_KEYS appears more than once" >&2
  grep -nE '^(# )?CONFIG_SYSTEM_TRUSTED_KEYS' "$CFG" || true
  exit 1
else
  echo "PASS: SYSTEM_TRUSTED_KEYS appears exactly once"
fi

# 3) Sanity: defconfig contributions still present
assert_grep '^CONFIG_64BIT=y$' "$CFG" "defconfig: 64BIT present"
assert_grep '^CONFIG_X86_64=y$' "$CFG" "defconfig: X86_64 present"

echo "All real-merge assertions passed."
