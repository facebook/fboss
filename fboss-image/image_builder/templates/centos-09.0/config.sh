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

# 5. Generate a fix-nvme script that "may" need to be run
MODULE_DIR="/usr/lib/dracut/modules.d/99nvme-fix"
mkdir -p "$MODULE_DIR"

# 5a. Generate the script directly in the target directory
cat >"$MODULE_DIR/fix-nvme.sh" <<'EOF'
#!/bin/bash
# Force all NVMe drives to 512e mode for KIWI compatibility if they are
# not already in that size

DEV=/dev/nvme0n1

# Only run if the device node exists
if [ -b "$DEV" ]; then
  # Check current logical block size
  if [ -f "/sys/class/block/nvme0n1/queue/logical_block_size" ]; then
    LB_SIZE=$(cat /sys/class/block/nvme0n1/queue/logical_block_size)
  fi

  # If != 512 block size, we have a mismatch
  if [ "$LB_SIZE" != "512" ]; then
    echo "NVMe-Fix: ${LB_SIZE} NVMe block size detected. Attempting format to 512..." >&2

    # Find the ID of the 512-byte format (usually 0 or 1)
    # We look for "Data Size: 512" in the output:
    #
    #    [root@fboss ~]# nvme id-ns $DEV -H | grep "Data Size:"
    #    LBA Format  0 : Metadata Size: 0   bytes - Data Size: 512 bytes - Relative Performance: 0x2 Good (in use)
    #    LBA Format  1 : Metadata Size: 0   bytes - Data Size: 4096 bytes - Relative Performance: 0x1 Better
    #    [root@fboss ~]#

    FMT_ID=$(nvme id-ns $DEV -H | awk '/Data Size:.*512/ {print $3; exit 0}')

    if [ -n "$FMT_ID" ]; then
      echo "NVMe-Fix: Formatting $DEV with LBA Format $FMT_ID..." >&2
      # DANGER: This wipes the drive!
      nvme format --lbaf=$FMT_ID --force $DEV

      # Wait for the drive to reset
      sleep 2
      udevadm settle
      blockdev --rereadpt $DEV
      echo "NVMe-Fix: Format complete." >&2
    else
      echo "NVMe-Fix: ERROR - No 512-byte format supported by this drive." >&2
    fi
  fi
fi
EOF

# 5b. Make the hook executable
chmod +x "$MODULE_DIR/fix-nvme.sh"

# 5c. Generate the module-setup.sh
cat >"$MODULE_DIR/module-setup.sh" <<'EOF'
#!/bin/bash

check() {
  # Always run this module
  return 0
}

depends() {
  # No complex dependencies
  return 0
}

install() {
  # Ensure these tools are inside the initrd
  inst_multiple nvme grep awk head cat sleep udevadm blockdev

  # Install the hook script to run BEFORE mounting (pre-mount priority 00)
  # $moddir resolves to the directory where this script sits
  inst_hook pre-mount 00 "$moddir/fix-nvme.sh"
}
EOF

# 5d. Make the setup script executable
chmod +x "$MODULE_DIR/module-setup.sh"

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
