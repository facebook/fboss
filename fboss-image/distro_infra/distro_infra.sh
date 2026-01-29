#!/bin/bash
set -e

INTERFACE=""
PERSIST_DIR=""
NODHCPV6=""

help() {
  echo "Usage: $0 [--nodhcpv6] --intf <interface> --persist-dir <persistent dir>"
  echo ""
  echo "Options:"
  echo ""
  echo "  -i|--intf <interface>       Network interface to attach to (must have L2 adjacency with FBOSS duts)"
  echo "  -p|--persist-dir <dir>      Directory for persistent storage, primarily of images to load"
  echo "  -n|--nodhcpv6               Do not provide DHCPv6, rely on external DHCPv6 server"
  echo ""
  echo "  -h|--help                   Print this help message"
  echo ""
  echo "Examples:"
  echo "  $0 --intf vlan1033 --persist-dir persist"
  echo "  $0 -i vlan1033 -p persist"
  echo ""
}

if [[ $# -eq 0 ]]; then
  echo "Error: No arguments provided"
  help
  exit 1
else
  while [[ $# -gt 0 ]]; do
    case "$1" in
    -i | --intf)
      INTERFACE="$2"
      shift 2
      ;;
    -p | --persist-dir)
      PERSIST_DIR="$2"
      shift 2
      ;;
    -n | --nodhcpv6)
      NODHCPV6="--nodhcpv6"
      shift 1
      ;;
    -h | --help)
      help
      exit 0
      ;;
    *)
      echo "Error: Unrecognized command option: '${1}'"
      help
      exit 1
      ;;
    esac
  done
fi

if [[ -z $INTERFACE || -z $PERSIST_DIR ]]; then
  echo "Error: --intf and --persist-dir are required"
  help
  exit 1
fi

mkdir -p "${PERSIST_DIR}"

# Run the Docker container with the parsed arguments
docker run --rm -it --network host --cap-add=NET_ADMIN \
  --volume "$(realpath "${PERSIST_DIR}")":/distro_infra/persistent:rw \
  fboss_distro_infra /distro_infra/run_distro_infra.sh "${NODHCPV6}" --intf "${INTERFACE}"
