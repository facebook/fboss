#!/bin/bash

fboss_dir=$(realpath "$(dirname "$0")/../../..")
pushd $fboss_dir >/dev/null || exit 1

DOCKER_BUILDKIT=1 docker build . -t fboss_builder -f fboss/oss/docker/Dockerfile \
  --build-arg USE_CLANG=true
