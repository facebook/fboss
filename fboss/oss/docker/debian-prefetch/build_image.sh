#!/usr/bin/env bash

# Requires sudo permissions, and should be run from root of FBOSS OSS repo

if [ $# -eq 0 ]; then
    echo "Please provide the output path for the image tarball e.g. " \
      "build_image.sh path/to/output/fboss_docker_debian_prefetch.tar.zst"
    exit 1
fi

docker build . -t fboss_docker_debian -f fboss/oss/docker/Dockerfile.debian

docker build . -t fboss_docker_debian_prefetch -f fboss/oss/docker/debian-prefetch/Dockerfile

temp_dir=$(mktemp -d)
intermediate="$temp_dir/fboss_docker_debian_prefetch.tar"
final_output="$temp_dir/fboss_docker_debian_prefetch.tar.zst"

docker save -o "$intermediate" fboss_docker_debian_prefetch

zstd -T0 -o "$final_output" "$intermediate"

mv "$final_output" "$1"

rm -rf "$temp_dir"

echo "Image saved to $1"
