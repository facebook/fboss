#!/bin/bash
set -e

pushd parts/ipxe
./build.sh
popd

DOCKER_BUILDKIT=1 docker build . -t fboss_distro_infra
