#!/bin/bash
# Entrypoint script for device container
# This is called by systemd after boot

set -e

# Setup btrfs loopback filesystem if not already done
if [ ! -f /var/btrfs.img ]; then
  /usr/local/bin/setup_btrfs.sh
fi

echo "Device container ready"
