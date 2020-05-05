#!/usr/bin/env bash

if [[ -z ${FBOSS} ]]; then
  echo "FBOSS is unset. Run 'source ./bin/setup_fboss_env' to set it up."
  exit 1
else
  echo "Running setup.sh with FBOSS='$FBOSS'";
fi

# Copy fruid.json
if [ ! -f "/var/facebook/fboss/fruid.json" ]; then
    mkdir -p /var/facebook/fboss
    cp "$FBOSS_DATA/fruid.json" /var/facebook/fboss
fi

# Copy bde.conf
if [ ! -f "/etc/modprobe.d/bde.conf" ]; then
    cp "$FBOSS_DATA/bde.conf" /etc/modprobe.d/bde.conf
fi

# Unload, remove old BCM modules
modprobe -r linux-user-bde
modprobe -r linux-kernel-bde
rm -f "/lib/modules/$(uname -r)/linux-user-bde.ko"
rm -f "/lib/modules/$(uname -r)/linux-kernel-bde.ko"

# Copy and load new BCM kernel modules
ln -s "$FBOSS_KMODS/linux-user-bde.ko" -t "/lib/modules/$(uname -r)"
ln -s "$FBOSS_KMODS/linux-kernel-bde.ko" -t "/lib/modules/$(uname -r)"
depmod -a
modprobe linux-kernel-bde
modprobe linux-user-bde
