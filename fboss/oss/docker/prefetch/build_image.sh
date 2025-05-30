#!/usr/bin/env bash

# Requires sudo permissions, and should be run from root of FBOSS OSS repo

if [ $# -lt 2 ]; then
    echo "Please provide the output path for the image tarball e.g. " \
      "build_image.sh path/to/output/fboss_docker_prefetch.tar.zst debian"
    exit 1
fi

case "$2" in
    debian)
        echo "You have chosen Debian."
        BASE_IMAGE="fboss/oss/docker/Dockerfile.debian"
        ;;
    centos)
        echo "You have chosen CentOS."
        BASE_IMAGE="fboss/oss/docker/Dockerfile"
        ;;
    *)
        echo "Error: Invalid distribution. Only 'debian' and 'centos' are supported."
        exit 1
        ;;
esac

docker build . -t fboss_docker_base -f $BASE_IMAGE

docker build . -t fboss_docker_prefetch -f fboss/oss/docker/prefetch/Dockerfile

temp_dir=$(mktemp -d)
intermediate="$temp_dir/fboss_docker_prefetch.tar"
final_output="$temp_dir/fboss_docker_prefetch.tar.zst"

docker save -o "$intermediate" fboss_docker_prefetch

zstd -T0 -o "$final_output" "$intermediate"

mv "$final_output" "$1"

rm -rf "$temp_dir"

echo "Image saved to $1"
