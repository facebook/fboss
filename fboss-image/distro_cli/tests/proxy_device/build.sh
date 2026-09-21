#!/bin/bash
set -e

cd "$(dirname "$0")"

DOCKER_BUILDKIT=1 docker build . -t fboss_proxy_device
