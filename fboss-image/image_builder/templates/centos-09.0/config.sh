#!/bin/bash

echo "--- Executing $0 ---"

# Modify the PRETTY_NAME entry in /usr/lib/os-release
sed -i 's/^PRETTY_NAME=.*/PRETTY_NAME="FBOSS Distro Image"/' /usr/lib/os-release
sed -i 's/^NAME=.*/NAME="FBOSS Distro Image"/' /usr/lib/os-release

# 1. Install our custom kernel RPMs
#
# On purpose we don't install any kernel rpms as part of
# config.xml as that picks the default that comes with
# the CentOS Stream 9 and is very old (5.14). We install
# kernel rpm present in /repos directory. This will either
# be the LTS 6.12 or whatever the user specified.
echo "Installing kernel rpms..."
dnf install --disablerepo=* -y  /repos/*.rpm

# 2. Define paths
# Detect the installed kernel version from the boot directory
# shellcheck disable=SC2012
VMLINUZ_PATH=$(ls /boot/vmlinuz-* 2>/dev/null | head -n 1)
if [ -z "$VMLINUZ_PATH" ]; then
    echo "Error: No vmlinuz found in /boot"
    exit 1
fi

KERNEL_VERSION=$(basename "$VMLINUZ_PATH" | sed 's/^vmlinuz-//')
# Detect the initrd path from the boot directory
# shellcheck disable=SC2012
INITRD_PATH=$(ls /boot/initramfs-* 2>/dev/null | head -n 1)
if [ -z "$INITRD_PATH" ]; then
    echo "Error: No initramfs found in /boot"
    exit 1
fi

echo "Detected kernel version: ${KERNEL_VERSION}"
echo "Vmlinuz path: ${VMLINUZ_PATH}"
echo "Initrd path: ${INITRD_PATH}"

# 3. Manually run dracut to create the initrd
#    --force is needed to overwrite any existing file
#    --kver specifies the kernel version to build for
echo "Running dracut manually..."
dracut --force --kver "${KERNEL_VERSION}" "${INITRD_PATH}"

# 4. Run kernel-install for grub config
# This wipes ALL interfering variables set by kiwi-ng
# and runs kernel-install in a "sterile" environment.
env -i \
    PATH="/usr/bin:/usr/sbin:/bin:/sbin" \
    kernel-install add "${KERNEL_VERSION}" "${VMLINUZ_PATH}" --initrd-file "${INITRD_PATH}"

# 6. Done! Cleanup, remember that we are chrooted on the rootfs
echo "Removing kernel rpms from rootfs..."
rm -f /repos/*.rpm
rmdir /repos

echo "Custom kernel ${KERNEL_VERSION} install complete."
exit 0
