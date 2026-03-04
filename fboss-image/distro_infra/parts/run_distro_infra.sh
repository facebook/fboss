#!/bin/bash

INTERFACE=""

help() {
  echo "Usage: $0 --intf <interface>"
  echo ""
  echo "Run FBOSS Distro Infrastructure PXE boot services"
  echo ""
  echo "Options:"
  echo ""
  echo "  -i|--intf <interface>       Network interface to attach to (must have L2 adjacency with FBOSS duts)"
  echo ""
  echo "  -h|--help                   Print this help message"
  echo ""
  echo "Examples:"
  echo "  $0 --intf vlan1033"
  echo "  $0 -i eth0"
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

if [[ -z $INTERFACE ]]; then
  echo "Error: --intf is required"
  help
  exit 1
fi

v4_ip=$(ip -4 addr show dev "$INTERFACE" | awk -F '[[:space:]/]+' '/inet/{print $3}')
v6_ip=$(ip -6 addr show dev "$INTERFACE" scope global | awk -F '[[:space:]/]+' '/inet6/{print $3; exit}')
echo "Listening on ${INTERFACE} - ${v4_ip} & ${v6_ip}"

mkdir -m 777 /distro_infra/persistent/cache 2>/dev/null
cp /distro_infra/ipxev4.efi /distro_infra/persistent/cache
cp /distro_infra/ipxev6.efi /distro_infra/persistent/cache
cp /distro_infra/autoexec.ipxe /distro_infra/persistent/cache

sed -i "s/V4IP/${v4_ip}/" /distro_infra/nginx.conf
sed -i "s/V6IP/${v6_ip}/" /distro_infra/nginx.conf
nginx -c /distro_infra/nginx.conf -p /distro_infra/persistent

# Minimize responding to other devices
echo 'tag:!fbossdut,ignore' >/distro_infra/dnsmasq_conf.d/default_ignore

dnsmasq --interface="${INTERFACE}" --no-daemon \
  --port=0 \
  --enable-tftp \
  --tftp-root=/distro_infra/persistent \
  --tftp-unique-root=mac \
  --tftp-secure \
  --dhcp-script=/distro_infra/post_tftp.sh \
  --dhcp-hostsdir=/distro_infra/dnsmasq_conf.d \
  --dhcp-range=tag:fbossdut,"${v4_ip}",proxy \
  --pxe-service=tag:fbossdut,x86-64_EFI,ipxe,ipxev4.efi \
  --enable-ra \
  --dhcp-range=tag:fbossdut,::fb05:5000:0001,::fb05:50ff:ffff,constructor:"$INTERFACE",5m \
  --dhcp-option=tag:fbossdut,option6:bootfile-url,tftp://["${v6_ip}"]/ipxev6.efi &

# Block on dnsmasq running in the background
wait
