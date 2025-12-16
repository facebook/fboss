#!/bin/bash

# Script that builds a PXE and USB bootable ISO an image using kiwi-ng-3.

# Get the full path to the workspace root directory where everything lives
WSROOT=$(cd "$(dirname "$0")/.." && pwd)

# Change directory full path to correct levels up from the script location so that we can include
# the functions.sh file
LOG_DIR="${WSROOT}/logs"

# Source common functions
# shellcheck disable=SC1091
source "${WSROOT}/lib/functions.sh"

# Save all arguments for later use
ORIGINAL_ARGS=("$0" "$@")

# Default values
DESCRIPTION_DIR="${WSROOT}/templates/centos-09.0"
TARGET_DIR="${WSROOT}/output"

# User configurable variables (fboss tarfile and kernel rpm directory)
FBOSS_TARFILE=""
KERNEL_RPM_DIR="" # default: download LTS 6.12

mkdir -p "${LOG_DIR}"
LOG_FILE="${LOG_DIR}/build_image_in_container.log"

help() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo ""
  echo "  -f|--fboss-tarfile          Location of compressed FBOSS tar file to add to image"
  echo "  -k|--kernel-rpm-dir         Directory containing kernel rpms to install (default: download LTS 6.12)"
  echo ""
  echo "  -h|--help                   Print this help message"
  echo ""
}

# Once this is finalized, it may be better to use a Dockerfile to build the image
update_docker() {
  dnf install -y \
    epel-release \
    kiwi \
    policycoreutils \
    python3-kiwi \
    dracut-kiwi-live \
    dracut-kiwi-overlay \
    dnf-plugin-versionlock \
    dracut-kiwi-oem-dump \
    kiwi-systemdeps-image-validation \
    syslinux \
    btrfs-progs
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in

  -f | --fboss-tarfile)
    FBOSS_TARFILE=$2
    shift 2
    ;;

  -k | --kernel-rpm-dir)
    KERNEL_RPM_DIR=$2
    shift 2
    ;;

  -h | --help)
    help
    exit 0
    ;;
  *)
    echo "Unrecognized command option: '${1}'"
    exit 1
    ;;
  esac
done

# Log everything for posterity ;-)
true >"${LOG_FILE}" # Truncate log file
export LOG_FILE

dprint "Script launch cmdline: ${ORIGINAL_ARGS[*]}"
dprint " ... logging all output to: ${LOG_FILE}"
dprint " ... output directory: ${TARGET_DIR}"

# Perform some sanity checks of user input
if [ -n "${KERNEL_RPM_DIR}" ] && [ ! -d "${KERNEL_RPM_DIR}" ]; then
  dprint "ERROR: directory of kernel rpms: ${KERNEL_RPM_DIR} not accessible, exiting..."
  exit 1
fi

if [ -n "${FBOSS_TARFILE}" ]; then
  if [ ! -f "${FBOSS_TARFILE}" ]; then
    dprint "ERROR: FBOSS tarfile: ${FBOSS_TARFILE} not accessible, exiting..."
    exit 1
  fi
fi

# Update the docker image
dprint "Updating docker image..."
update_docker >>"${LOG_FILE}" 2>&1

dprint "Deleting target directory: ${TARGET_DIR}"
rm -rf "${TARGET_DIR}"

# Create the output directory (in case it doesn't exist)
mkdir -p "${TARGET_DIR}"
chmod 777 "${TARGET_DIR}"

# If you place a tar file in the description directory, it will be automatically extracted and
# overlayed on top of the root file system.  We use this to deploy FBOSS binaries that are generated
# in a different build process.
rm -f "${DESCRIPTION_DIR}/root.tar.gz" # Remove any existing tar file
if [ -n "${FBOSS_TARFILE}" ]; then
  if [ -f "${FBOSS_TARFILE}" ]; then
    dprint "Copying ${FBOSS_TARFILE} to ${DESCRIPTION_DIR}/root.tar.gz ..."
    cp "${FBOSS_TARFILE}" "${DESCRIPTION_DIR}/root.tar.gz"
  else
    dprint "ERROR: ${FBOSS_TARFILE} does not exist, exiting..."
    exit 1
  fi
fi

# Create a directory to hold the kernel rpms
rm -rf "${DESCRIPTION_DIR}/root/repos"
mkdir -p "${DESCRIPTION_DIR}/root/repos"

# If the user specified a kernel rpm directory, copy the rpms from there.  Otherwise, download them
if [ -z "${KERNEL_RPM_DIR}" ]; then
  dprint "Downloading LTS kernel 6.12 rpms to ${DESCRIPTION_DIR}/root/repos..."
  dnf copr enable kwizart/kernel-longterm-6.12 -y >>"${LOG_FILE}" 2>&1
  if ! dnf download --destdir="${DESCRIPTION_DIR}/root/repos" kernel-longterm-core-* kernel-longterm-modules-core-* >>"${LOG_FILE}" 2>&1; then
    dprint "ERROR: Failed to download LTS kernel rpms, check logfile for errors, exiting..."
    exit 1
  fi
else
  dprint "Copying kernel rpms to ${DESCRIPTION_DIR}/root/repos..."
  for rpm_file in "${KERNEL_RPM_DIR}"/*.rpm; do
    if [ -f "$rpm_file" ]; then
      dprint "   ... $(basename "$rpm_file")"
      cp "$rpm_file" "${DESCRIPTION_DIR}/root/repos/"
    fi
  done
fi

dprint "Copying /etc/resolv.conf to ${DESCRIPTION_DIR}/root/etc/resolv.conf..."
# Pass /etc/resolv.conf to the chrooted environment
mkdir -p "${DESCRIPTION_DIR}/root/etc"
cp /etc/resolv.conf "${DESCRIPTION_DIR}/root/etc/"

# Add build timestamp to the image
echo "Built on: $(date -u)" >"$DESCRIPTION_DIR/root/etc/build-info"

# Generate the images
dprint "Generating PXE and USB bootable image, this will take few minutes..."
kiwi-ng-3 \
  --profile FBOSS \
  --type oem \
  system build \
  --description "${DESCRIPTION_DIR}" \
  --target-dir "${TARGET_DIR}" \
  >>"${LOG_FILE}" 2>&1

RC=$?

if [ "${RC}" -eq 0 ]; then
  ls -l "${TARGET_DIR}"/FBOSS-Distro-Image.x86_64-1.0.install.*
fi

dprint "Image generation completed with exit code ${RC}"
exit "${RC}"
