#!/bin/bash

if [ -z "$1" -o -z "$2" -o "$1" == "--help" ]; then
    echo "Usage: $0 --help - Show this help"
    echo "       $0 <interface> <persistent directory> - Run Distro Infra container"
    exit 1
fi

docker run --rm -it --network host --cap-add=NET_ADMIN \
    --volume $(realpath ${2}):/distro_infra/persistent:rw \
    fboss_distro_infra /distro_infra/run_distro_infra.sh ${1}
