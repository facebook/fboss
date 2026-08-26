#!/bin/bash
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#
# On devservers, run with sudo for --network=host to work:
#   sudo -E ./fboss/oss/scripts/build_docker.sh
set -euo pipefail

fboss_dir=$(realpath "$(dirname "$0")/../../..")
pushd "$fboss_dir" >/dev/null || exit 1

EXTRA_ARGS=()
if [ "${FBOSS_META_ENV:-}" = "1" ]; then
    echo "[fboss] Meta devserver detected — run with: sudo -E $0"
    EXTRA_ARGS+=(--network=host
                 --build-arg http_proxy=http://fwdproxy:8080
                 --build-arg https_proxy=http://fwdproxy:8080
                 --build-arg GITHUB_ROOT=fboss/github
                 --build-arg FBCODE_BUILDER_PATH=opensource/fbcode_builder
                 --build-arg FBOSS_META_ENV=1)
fi

DOCKER_BUILDKIT=1 docker build . -t fboss_builder -f fboss/oss/docker/Dockerfile \
  --build-arg USE_CLANG=true \
  "${EXTRA_ARGS[@]}"
