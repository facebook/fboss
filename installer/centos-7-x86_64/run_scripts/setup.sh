#!/usr/bin/env bash

# Copy fruid.json
if [ ! -f "/var/facebook/fboss/fruid.json" ]; then
    mkdir -p /var/facebook/fboss
    cp run_scripts/fruid.json /var/facebook/fboss
fi

# Copy bde.conf
if [ ! -f "/etc/modprobe.d/bde.conf" ]; then
    cp run_scripts/bde.conf /etc/modprobe.d/bde.conf
fi

# Unload, remove old BCM modules
modprobe -r linux-user-bde
modprobe -r linux-kernel-bde
rm -f "/lib/modules/$(uname -r)/linux-user-bde.ko"
rm -f "/lib/modules/$(uname -r)/linux-kernel-bde.ko"

# Copy and load new BCM kernel modules
ln -s "$PWD/linux-user-bde.ko" -t "/lib/modules/$(uname -r)"
ln -s "$PWD/linux-kernel-bde.ko" -t "/lib/modules/$(uname -r)"
depmod -a
modprobe linux-kernel-bde
modprobe linux-user-bde
