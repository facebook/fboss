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

# 6. Done! Cleanup, remember that we are chrooted on the rootfs
echo "Removing kernel rpms from rootfs..."
rm -f /repos/*.rpm
rmdir /repos

echo "Custom kernel ${KERNEL_VERSION} install complete."
exit 0
