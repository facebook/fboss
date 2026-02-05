#!/bin/bash

echo "--- Executing $0 ---"

# Modify the PRETTY_NAME entry in /usr/lib/os-release
sed -i 's/^PRETTY_NAME=.*/PRETTY_NAME="FBOSS Distro Image"/' /usr/lib/os-release
sed -i 's/^NAME=.*/NAME="FBOSS Distro Image"/' /usr/lib/os-release

# All dnf invocations with an invalid RPM repo configured will fail. Create the
# metadata for the local_rpm_repo now to prevent that.
createrepo /usr/local/share/local_rpm_repo

# 1. Install our custom kernel RPMs
#
# On purpose we don't install any kernel rpms as part of
# config.xml as that picks the default that comes with
# the CentOS Stream 9 and is very old (5.14). We install
# kernel rpm present in /repos directory. This will either
# be the LTS 6.12 or whatever the user specified.
echo "Installing kernel rpms..."
dnf install --disablerepo=* -y /repos/*.rpm

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

# 5. Enable systemd services
echo "Enabling FBOSS systemd services..."
systemctl enable local_rpm_repo.service
systemctl enable platform_manager.service
systemctl enable data_corral_service.service
systemctl enable fan_service.service
systemctl enable sensor_service.service
systemctl enable fsdb.service
systemctl enable qsfp_service.service
systemctl enable wedge_agent.service

# 6. Use system GRUB 2.06 from packages
# The grub2-efi-x64 package already provides grubx64.efi with all necessary modules
# We just need to make sure the btrfs module is accessible on the EFI partition
echo "Using system GRUB 2.06 from grub2-efi-x64 package..."

# Clean up any old GRUB module directories from any previous builds
# (The grub2-efi-x64 package creates /boot/grub2/ and /boot/efi/EFI/ but not the module directories)
echo "Cleaning up old GRUB module directories..."
rm -rf /boot/grub/x86_64-efi /boot/grub2/x86_64-efi
echo "Old GRUB module directories cleaned"

# Copy GRUB modules to multiple locations
# EFI partition structure: (hd0,gpt2)/efi/centos/x86_64-efi/ and (hd0,gpt2)/efi/boot/x86_64-efi/
# During build, EFI partition is mounted at /boot/efi/, so:
#   (hd0,gpt2)/efi/centos/x86_64-efi/ -> /boot/efi/efi/centos/x86_64-efi/
#   (hd0,gpt2)/efi/boot/x86_64-efi/ -> /boot/efi/efi/boot/x86_64-efi/
#
# Essential modules needed for early boot (to find and mount btrfs root) dependency tree:
#   btrfs -> zstd, lzopio, extcmd, raid6rec, gzio
#   lzopio -> crypto
#   raid6rec -> diskfilter
#   gzio -> gcry_crc, crypto
ESSENTIAL_MODULES="btrfs.mod zstd.mod lzopio.mod extcmd.mod raid6rec.mod gzio.mod crypto.mod diskfilter.mod gcry_crc.mod"

# Copy essential modules to EFI partition locations
for efi_dir in /boot/efi/efi/centos/x86_64-efi /boot/efi/efi/boot/x86_64-efi; do
  mkdir -p "$efi_dir"
  for mod in $ESSENTIAL_MODULES; do
    cp /usr/lib/grub/x86_64-efi/$mod "$efi_dir/" 2>/dev/null || echo "Warning: $mod not found"
  done
  echo "Copied essential GRUB modules to $efi_dir (EFI partition)"
done

# Copy ALL modules to root partition (plenty of space there)
mkdir -p /boot/grub2/x86_64-efi
cp -r /usr/lib/grub/x86_64-efi/* /boot/grub2/x86_64-efi/
echo "Copied all GRUB modules to /boot/grub2/x86_64-efi/ (root partition)"

# 7. Done! Cleanup, remember that we are chrooted on the rootfs
echo "Removing kernel rpms from rootfs..."
rm -f /repos/*.rpm
rmdir /repos

echo "Custom kernel ${KERNEL_VERSION} install complete."
exit 0
