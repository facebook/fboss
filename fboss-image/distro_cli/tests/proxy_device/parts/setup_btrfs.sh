#!/bin/bash
# Setup btrfs loopback filesystem with base snapshot and per-stack volumes
set -euo pipefail

# Remove nologin file to allow SSH login (systemd creates this during boot)
rm -f /run/nologin /var/run/nologin

BTRFS_IMG=/var/btrfs.img
BTRFS_MOUNT=/mnt/btrfs

# Skip btrfs setup if already mounted (but nologin removal above always runs)
if mountpoint -q "$BTRFS_MOUNT" 2>/dev/null; then
  echo "btrfs already mounted, skipping"
  exit 0
fi
DISTRO_BASE=/distro-base

# Clean up any stale loop devices from previous container runs
# (loop devices are shared with host and persist after container exit)
for dev in $(losetup -j "$BTRFS_IMG" 2>/dev/null | cut -d: -f1); do
  losetup -d "$dev" 2>/dev/null || true
done

# Clean up any existing file from previous failed runs
rm -f $BTRFS_IMG

# Create loopback file (1GB to accommodate full root filesystem copy)
dd if=/dev/zero of=$BTRFS_IMG bs=1M count=1024

# Set up loop device first, then format and mount
mkdir -p $BTRFS_MOUNT
LOOP_DEV=$(losetup -f --show $BTRFS_IMG)
mkfs.btrfs $LOOP_DEV
mount $LOOP_DEV $BTRFS_MOUNT

# Copy the container's root filesystem to btrfs (mimics ONIE extracting rootfs)
# Exclude special filesystems and the btrfs image itself to avoid recursion
echo "Copying root filesystem to btrfs..."
rsync -aAX \
  --exclude=/dev/* \
  --exclude=/proc/* \
  --exclude=/sys/* \
  --exclude=/run/* \
  --exclude=/mnt/* \
  --exclude=/tmp/* \
  --exclude=/var/btrfs.img \
  / $BTRFS_MOUNT/

# Create base snapshot from the root copy (mimics real device installation)
# This is what install.sh.tmpl does: btrfs subvolume snapshot ${demo_mnt} ${demo_mnt}/distro-base
echo "Creating base snapshot..."
btrfs subvolume snapshot $BTRFS_MOUNT $BTRFS_MOUNT/distro-base

# Fix /var/run symlink: replace with real directory so services can write version files
# when running with RootDirectory isolation
if [ -L $BTRFS_MOUNT/distro-base/var/run ]; then
  rm $BTRFS_MOUNT/distro-base/var/run
  mkdir -p $BTRFS_MOUNT/distro-base/var/run
fi

# Create symlinks for easy access (remove existing dirs first)
rm -rf $DISTRO_BASE /updates
ln -sf $BTRFS_MOUNT/distro-base $DISTRO_BASE

# Create updates directory on btrfs
mkdir -p $BTRFS_MOUNT/updates
ln -sf $BTRFS_MOUNT/updates /updates

echo "Base snapshot created at $DISTRO_BASE (snapshot of root filesystem)"

# Create initial per-service subvolumes (mimics first update)
# This allows integration tests to verify services run in subvolumes
echo "Creating initial service subvolumes..."
SERVICES="wedge_agent fsdb qsfp_service platform_manager sensor_service fan_service data_corral_service"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

for svc in $SERVICES; do
  SUBVOL_PATH="$BTRFS_MOUNT/updates/${svc}-${TIMESTAMP}"
  echo "  Creating subvolume for $svc at $SUBVOL_PATH"
  btrfs subvolume snapshot $BTRFS_MOUNT/distro-base "$SUBVOL_PATH"

  # Convert /var/run symlink to real directory so version files persist in subvolume
  # (by default /var/run -> ../run which points to tmpfs outside the subvolume)
  rm -f "$SUBVOL_PATH/var/run"
  mkdir -p "$SUBVOL_PATH/var/run"

  # Create systemd drop-in to run service in its subvolume
  DROPIN_DIR="/etc/systemd/system/${svc}.service.d"
  mkdir -p "$DROPIN_DIR"
  cat >"$DROPIN_DIR/root-override.conf" <<EOF
[Service]
RootDirectory=$SUBVOL_PATH
EOF
done

# Reload systemd to pick up the drop-ins
systemctl daemon-reload

echo "btrfs setup complete: Base=$DISTRO_BASE, Services in subvolumes"
