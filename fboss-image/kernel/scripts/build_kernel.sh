#!/usr/bin/env bash
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
set -euo pipefail

# Internal script to build kernel RPMs inside the container.
# Args:
#   $1: Kernel version (required)
#   $2: Output directory inside container (required), e.g. /output

KERNEL_VERSION="${1:?kernel version required}"
OUT_DIR="${2:?output dir required}"

# Compute container paths (avoid relying on host-exported env)
CONTAINER_WORKSPACE="/var/FBOSS/fboss"
KERNEL_ROOT="$CONTAINER_WORKSPACE/fboss-image/kernel"
CONTAINER_DIST_DIR="$KERNEL_ROOT/dist"
CONTAINER_SPECS_DIR="$KERNEL_ROOT/specs"
CONTAINER_CONFIGS_DIR="$KERNEL_ROOT/configs"

CONTAINER_SCRIPTS_DIR="$KERNEL_ROOT/scripts"

# Install kernel build dependencies
bash "$CONTAINER_SCRIPTS_DIR/setup_kernel_build_deps.sh"

# Use a separate build directory to avoid cluttering dist/
BUILD_DIR="$CONTAINER_DIST_DIR/build-$KERNEL_VERSION"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/SOURCES"
cd "$BUILD_DIR"

# Download kernel source (spectool is part of rpmdevtools)
spectool -g -C SOURCES "$CONTAINER_SPECS_DIR/kernel.spec" \
    --define "kernel_version $KERNEL_VERSION"

# Ensure FBOSS config sources are present for rpmbuild
cp "$CONTAINER_CONFIGS_DIR/fboss-reference.config" "$BUILD_DIR/SOURCES/"

# Generate version-specific local overrides from YAML
python3 "$CONTAINER_SCRIPTS_DIR/generate_config_overrides.py" \
    "$CONTAINER_CONFIGS_DIR/fboss-local-overrides.yaml" \
    "$KERNEL_VERSION" \
    "$BUILD_DIR/SOURCES/fboss-local-overrides.config"

rpmbuild -ba "$CONTAINER_SPECS_DIR/kernel.spec" \
    --define "_topdir $BUILD_DIR" \
    --define "_sourcedir $BUILD_DIR/SOURCES" \
    --define "kernel_version $KERNEL_VERSION"

# Copy RPMs to output directory
cp -r RPMS/* "$OUT_DIR"/ 2>/dev/null
cp -r SRPMS/* "$OUT_DIR"/ 2>/dev/null

tar czf $OUT_DIR/kernel-$KERNEL_VERSION.rpms.tar.gz \
    --transform 's|.*/||' \
    --transform 's|^\(kernel-[^-]\+\)-.*\.\(x86_64\)\.rpm$|\1-\2.rpm|' \
    $(find RPMS -name "*.rpm")

echo 'Kernel RPM build complete!'
echo 'Output files:'
find "$OUT_DIR" \( -name '*.rpm' -o -name '*.gz' \) -type f
