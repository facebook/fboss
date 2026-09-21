#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
# Sets up a Meta devserver for building FBOSS OSS directly from fbsource.
#
# What it does:
#   1. Exports FBOSS_META_ENV=1 (signals Meta environment to build scripts)
#   2. Installs podman-docker if not already installed
#
# Usage — add to .bashrc/.bash_profile:
#   source /path/to/fbsource/fbcode/fboss/oss/scripts/setup_devserver_oss_build.sh

# Signal Meta environment to build scripts (Dockerfile, build_docker.sh, entrypoint.sh)
export FBOSS_META_ENV=1

# --- Install podman-docker if needed ---
if ! command -v docker &>/dev/null; then
    echo "[fboss-devserver] docker not found. Installing podman-docker..."
    sudo dnf install -y podman-docker
fi
