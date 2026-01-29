#!/bin/bash

echo "--- Executing $0 ---"

LOCAL_RPM_REPO_DIR="/usr/local/share/local_rpm_repo"
mkdir -p "$LOCAL_RPM_REPO_DIR"

# Modify the PRETTY_NAME entry in /usr/lib/os-release
sed -i 's/^PRETTY_NAME=.*/PRETTY_NAME="FBOSS Distro Image"/' /usr/lib/os-release
sed -i 's/^NAME=.*/NAME="FBOSS Distro Image"/' /usr/lib/os-release

# All dnf invocations with an invalid RPM repo configured will fail. Create the
# metadata for the local_rpm_repo now to prevent that.
createrepo /usr/local/share/local_rpm_repo

# 1. Process component artifacts and install RPMs
#
# Component artifacts are copied to /repos/<component_name>/
# Each component is processed in isolation to avoid conflicts

process_kernel() {
  local component_dir=$1
  local component_tmp=$2

  echo "  Processing kernel component..."

  # Discover kernel tarballs (nullglob is enabled in the caller loop)
  local tarballs=("$component_dir"/*.tar*)

  if [ ${#tarballs[@]} -eq 0 ]; then
    echo "  No kernel tarballs found in $component_dir, skipping kernel install"
    return 0
  fi

  if [ ${#tarballs[@]} -gt 1 ]; then
    echo "ERROR: Found multiple kernel tarballs in $component_dir:"
    for tb in "${tarballs[@]}"; do
      echo "  - $(basename "$tb")"
    done
    echo "Only a single kernel tarball is supported"
    return 1
  fi

  local tarball="${tarballs[0]}"

  echo "  Extracting $(basename "$tarball") (excluding devel/header RPMs)..."
  tar -xf "$tarball" -C "$component_tmp" \
    --exclude='*-devel-*.rpm' \
    --exclude='*-headers-*.rpm'

  # Copy any unarchived RPMs that may already be in the component directory
  if ls "$component_dir"/*.rpm >/dev/null 2>&1; then
    cp "$component_dir"/*.rpm "$component_tmp/"
  fi

  # Install RPMs
  if ls "$component_tmp"/*.rpm >/dev/null 2>&1; then
    echo "  Installing kernel RPMs..."
    dnf install --disablerepo=* -y "$component_tmp"/*.rpm
  else
    echo "  No RPMs found for kernel"
  fi

  return 0
}

process_sai_tarball() {
  local component_dir=$1
  local component_name
  component_name=$(basename "$component_dir")

  # Discover SAI tarballs
  local tarballs=("$component_dir"/*.tar*)

  if [ ${#tarballs[@]} -eq 0 ]; then
    echo "  No SAI tarballs found in $component_dir, skipping SAI processing"
    return 0
  fi

  if [ ${#tarballs[@]} -gt 1 ]; then
    echo "ERROR: Found multiple SAI tarballs in $component_dir:"
    for tb in "${tarballs[@]}"; do
      echo "  - $(basename "$tb")"
    done
    echo "Only a single SAI tarball is supported"
    return 1
  fi

  local tarball="${tarballs[0]}"

  set -x
  # Extract only sai-runtime.rpm from the tarball
  echo "  Extracting sai-runtime.rpm from $(basename "$tarball")..."
  tar -xf "$tarball" -C "$component_dir" 'sai-runtime.rpm'

  # Check if the file was extracted successfully
  if [ -f "$component_dir/sai-runtime.rpm" ]; then
    echo "  Installing $component_dir/sai-runtime.rpm..."
    dnf install -y $component_dir/sai-runtime.rpm
    if [ $? -ne 0 ]; then
      echo "ERROR: Failed to install $component_dir/sai-runtime.rpm"
      return 1
    fi
  else
    echo "No sai-runtime.rpm found in $tarball"
  fi

  rm -f "$tarball"
  return 0

}

install_component_rpms() {
  local component_dir=$1
  local component_name
  component_name=$(basename "$component_dir")

  echo "  Installing RPMs for $component_name..."
  dnf install -y "$component_dir"/*.rpm

  return $?
}

echo "Processing component artifacts..."

shopt -s nullglob
for component_dir in /repos/*; do
  [ -d "$component_dir" ] || continue
  component_name=$(basename "$component_dir")

  handler_rc=0

  case "$component_name" in
  kernel)
    echo "Processing component: $component_name"

    # Create temporary directory for this component
    component_tmp=$(mktemp -d)

    process_kernel "$component_dir" "$component_tmp"
    handler_rc=$?

    # Clean up temp directory
    rm -rf "$component_tmp"
    ;;

  sai)
    # This is a tarfile with many files - just copy it to the local repo
    process_sai_tarball "$component_dir"
    handler_rc=$?
    ;;

  other_dependencies)
    install_component_rpms "$component_dir"
    handler_rc=$?
    ;;

  bsps)
    echo "Processing component: $component_name"
    for bsp in $component_dir/*.tar*; do
      echo "Processing BSP: $bsp"
      tar -C /usr/local/share/local_rpm_repo -xf "$bsp"
    done
    createrepo /usr/local/share/local_rpm_repo
    ;;

  fboss-forwarding-stack | fboss-platform-stack)
    echo "Processing component: $component_name"
    tarballs=("$component_dir"/*.tar*)
    if [ ${#tarballs[@]} -eq 0 ]; then
      echo "  No $component_name tarball found in $component_dir, skipping $component_name install"
    elif [ ${#tarballs[@]} -gt 1 ]; then
      echo "  Multiple $component_name tarballs found in $component_dir, skipping $component_name install"
    else
      tarball="${tarballs[0]}"
      echo "  Extracting $component_name tarball..."
      mkdir -p /opt/fboss
      tar -C /opt/fboss -xf "$tarball"
    fi
    ;;

  *)
    echo "Skipping component: $component_name (no handler defined)"
    ;;
  esac

  # Exit if handler failed
  if [ $handler_rc -ne 0 ]; then
    echo "ERROR: Failed to process $component_name"
    exit 1
  fi
done
shopt -u nullglob

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
echo "Custom kernel ${KERNEL_VERSION} install complete."

# 5. Configure SSH to allow password authentication and root login
echo "Configuring SSH..."
sed -i 's/^[# \t]*PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config
sed -i 's/^[# \t]*PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config

# 6. Generate a fix-nvme script that "may" need to be run
# --- Install Custom NVMe Fix Module (Inline Method) ---

MODULE_DIR="/usr/lib/dracut/modules.d/99nvme-fix"
mkdir -p "$MODULE_DIR"

# 6a. Generate the script directly in the target directory
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
  else
    echo "NVMe-Fix: Device ${DEV} already at 512-byte block size." >&2
  fi
fi
EOF

# 6b. Make the hook executable
chmod +x "$MODULE_DIR/fix-nvme.sh"

# 6c. Generate the module-setup.sh
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

# 6d. Make the setup script executable
chmod +x "$MODULE_DIR/module-setup.sh"

#-------------------------------------------------------
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

# 7. Enable systemd services
echo "Enabling FBOSS systemd services..."
systemctl enable local_rpm_repo.service
systemctl enable platform_manager.service
systemctl enable data_corral_service.service
systemctl enable fan_service.service
systemctl enable sensor_service.service
systemctl enable fsdb.service
systemctl enable qsfp_service.service
systemctl enable wedge_agent.service

# 8. Done! Cleanup and install additional packages
echo "Cleaning up /repos directory..."
rm -rf /repos

JQ_INSTALLED=false
# Ensure jq is installed (needed to parse JSON)
if ! rpm -q jq >/dev/null 2>&1; then
  dnf install -y jq
  JQ_INSTALLED=true
fi

#8. Install additional packages from after_pkgs input file user "may" have passed in
if [ -f /var/tmp/after_pkgs_install_file.json ]; then

  echo "Processing after_pkgs_install JSON file..."
  EXTRA_PACKAGES=$(jq -r '.packages[]' /var/tmp/after_pkgs_install_file.json)

  if [ -n "$EXTRA_PACKAGES" ]; then
    mapfile -t PACKAGE_ARRAY <<<"$EXTRA_PACKAGES"
    echo "Installing packages: ${PACKAGE_ARRAY[*]}"
    dnf install -y --setopt=install_weak_deps=False "${PACKAGE_ARRAY[@]}"

    # Clean up, don't leave traces of the user's JSON file in the image
    rm -f /var/tmp/after_pkgs_install_file.json
  else
    echo "No extra packages specified in after_pkgs_install JSON file"
  fi
else
  echo "No after_pkgs_install JSON file"
fi

#9. Excute additional commands from after_pkgs_execute_file.json user "may" have passed in
if [ -f /var/tmp/after_pkgs_execute_file.json ]; then

  echo "Processing after_pkgs_execute JSON file..."
  EXTRA_COMMANDS=$(jq -r '.execute[]' /var/tmp/after_pkgs_execute_file.json)

  if [ -n "$EXTRA_COMMANDS" ]; then
    # Get the number of commands in the execute array
    NUM_COMMANDS=$(jq -r '.execute | length' /var/tmp/after_pkgs_execute_file.json)

    for ((i = 0; i < NUM_COMMANDS; i++)); do
      # Extract the command array for this index
      CMD_ARRAY=$(jq -r ".execute[$i] | @sh" /var/tmp/after_pkgs_execute_file.json)

      echo "Executing command $((i + 1))/$NUM_COMMANDS: $CMD_ARRAY"

      # Execute the command using eval to properly handle the shell-quoted array
      eval "$CMD_ARRAY"

      if [ $? -ne 0 ]; then
        echo "Warning: Command $((i + 1)) failed with exit code != 0"
      fi
    done

    # Clean up, don't leave traces of the user's JSON file in the image
    rm -f /var/tmp/after_pkgs_execute_file.json
  else
    echo "No commands specified in after_pkgs_execute JSON file"
  fi
fi

# Clean up, remove jq if we installed it
if [ "$JQ_INSTALLED" = true ]; then
  dnf remove -y jq
fi

exit 0
