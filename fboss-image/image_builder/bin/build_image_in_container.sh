#!/bin/bash

# Script that builds a PXE and USB bootable ISO an image using kiwi-ng-3.

# For hardlinking component artifacts into the kiwi description tree we need
# both source and destination to live on the same filesystem. The kiwi
# description lives under /image_builder, so we expose the deps staging
# directory there and create a /deps symlink pointing into it. The
# /deps mount remains available for reading.
ln -sfn /image_builder/deps_staging /deps

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
KIWI_DEBUG=""
BUILD_PXE=""
BUILD_ONIE=""
AFTER_PKGS_INSTALL_FILE=""
AFTER_PKGS_EXECUTE_FILE=""

# User configurable variables
DEPS_DIR="" # Component artifacts directory

mkdir -p "${LOG_DIR}"
LOG_FILE="${LOG_DIR}/build_image_in_container.log"

help() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo ""
  echo "  --deps <dir>                Directory containing component artifacts (organized by component name)"
  echo "  -a|--after-pkgs-install     JSON File (in templates/centos-09.0 directory) containing additional packages to install to the image"
  echo "  -e|--after-pkgs-execute     JSON File (in templates/centos-09.0 directory) containing list of commands to execute after packages are installed"
  echo "  -p|--build-pxe-usb          Build PXE and USB installers image (default: no)"
  echo "  -o|--build-onie             Build ONIE installer image (default: no)"
  echo ""
  echo "  -d|--debug                  Enable kiwi-ng debug"
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
    btrfs-progs \
    glibc-static
}

build_zstd() {
  if [ ! -d ${TARGET_DIR}/zstd ]; then
    dprint "Building static zstd..."
    git clone https://github.com/facebook/zstd.git ${TARGET_DIR}/zstd
    pushd ${TARGET_DIR}/zstd >/dev/null
    git checkout release
    make -C programs zstd-frugal LDFLAGS="-static"
    strip programs/zstd-frugal
    popd >/dev/null
  fi
}

build_onie_installer() {
  build_zstd

  dprint "Creating ONIE installer..."
  out_bin=${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.install.bin
  rootfs=${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.tar.zst

  pushd ${TARGET_DIR}/onie >/dev/null

  mkdir -p onie_installer
  templates_dir="${WSROOT}/templates/onie"

  cp ${templates_dir}/distro-setup.sh.tmpl onie_installer/distro-setup.sh
  chmod a+x onie_installer/distro-setup.sh

  cp ${templates_dir}/default_platform.conf onie_installer/default_platform.conf
  chmod a+x onie_installer/default_platform.conf

  cp ${templates_dir}/install.sh.tmpl onie_installer/install.sh
  chmod a+x onie_installer/install.sh

  cp ${TARGET_DIR}/zstd/programs/zstd-frugal onie_installer/zstd
  chmod a+x onie_installer/zstd

  dprint "  Finding kernel and initrd..."
  kernel=$(
    set +o pipefail
    tar -tf $rootfs | awk -F / '/^boot\/vmlinuz/ {print $2; exit 0}' || exit 1
  )
  initrd=$(
    set +o pipefail
    tar -tf $rootfs | awk -F / '/^boot\/initramfs/ {print $2; exit 0}' || exit 1
  )

  sed -i -e "s/%%KERNEL_FILENAME%%/$kernel/g" \
    -e "s/%%INITRD_FILENAME%%/$initrd/g" \
    onie_installer/install.sh

  mv $rootfs onie_installer/rootfs.tar.zst

  dprint "  Packaging installer..."
  tar -cf installer.tar onie_installer

  sha1=$(cat installer.tar | sha1sum | awk '{print $1}')

  cp ${templates_dir}/sharch_body.sh ${out_bin}
  sed -i -e "s/%%IMAGE_SHA1%%/$sha1/" \
    -e "s/%%PAYLOAD_IMAGE_SIZE%%/$(stat -c %s installer.tar)/" \
    ${out_bin}

  cat installer.tar >>${out_bin}

  rm -rf installer.tar onie_installer

  popd >/dev/null
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in

  --deps)
    DEPS_DIR=$2
    shift 2
    ;;

  -p | --build-pxe-usb)
    BUILD_PXE="yes"
    shift 1
    ;;

  -o | --build-onie)
    BUILD_ONIE="yes"
    shift 1
    ;;

  -d | --debug)
    KIWI_DEBUG=" --debug "
    shift 1
    ;;

  -a | --after-pkgs-input-file)
    AFTER_PKGS_INSTALL_FILE=$2
    shift 2
    ;;

  -e | --after-pkgs-execute-file)
    AFTER_PKGS_EXECUTE_FILE=$2
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

# Perform some sanity checks of user input when an explicit --deps is provided
if [ -n "${DEPS_DIR}" ] && [ ! -d "${DEPS_DIR}" ]; then
  dprint "ERROR: deps directory: ${DEPS_DIR} not accessible, exiting..."
  exit 1
fi

# Update the docker image
dprint "Updating docker image..."
update_docker |& tee -a "${LOG_FILE}"

# Create the output directory (in case it doesn't exist)
mkdir -p "${TARGET_DIR}"
chmod 777 "${TARGET_DIR}"

rm -f "${DESCRIPTION_DIR}/root.tar.gz" # Remove any existing tar file

# Hardlink component artifacts to root/repos for processing in config.sh. When
# no explicit --deps is provided, use /deps (which resolves into the
# /image_builder filesystem) so that cp -la does not cross mount boundaries.
rm -rf "${DESCRIPTION_DIR}/root/repos"
mkdir -p "${DESCRIPTION_DIR}/root/repos"

EFFECTIVE_DEPS_DIR="${DEPS_DIR:-/deps}"

if [ -d "${EFFECTIVE_DEPS_DIR}" ] && [ -n "$(ls -A "${EFFECTIVE_DEPS_DIR}" 2>/dev/null)" ]; then
  dprint "Hardlinking component artifacts from ${EFFECTIVE_DEPS_DIR} to ${DESCRIPTION_DIR}/root/repos..."
  cp -al "${EFFECTIVE_DEPS_DIR}"/* "${DESCRIPTION_DIR}/root/repos/"
fi

dprint "Copying /etc/resolv.conf to ${DESCRIPTION_DIR}/root/etc/resolv.conf..."
# Pass /etc/resolv.conf to the chrooted environment
mkdir -p "${DESCRIPTION_DIR}/root/etc"
cp /etc/resolv.conf "${DESCRIPTION_DIR}/root/etc/"

# Add build timestamp to the image
echo "Built on: $(date -u)" >"$DESCRIPTION_DIR/root/etc/build-info"

# Copy rootfs template files to overlay
dprint "Copying rootfs files to overlay..."
cp -R ${DESCRIPTION_DIR}/root_files/* ${DESCRIPTION_DIR}/root/

# Remove any existing after_pkgs files from previous runs
rm -f ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_install_file.json
rm -f ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_execute_file.json

# Copy the after_pkgs input file in the rootfs so that we can access it the chrooted environment
# The file, if provided by the user, should already copied into the centos-09.0 directory as part
# of the image build process. Copy to fixed location in rootfs where config.sh checks for it.
# These will be deleted once they are processed.
if [[ -n ${AFTER_PKGS_INSTALL_FILE} && -f "${DESCRIPTION_DIR}/${AFTER_PKGS_INSTALL_FILE}" ]]; then
  mkdir -p ${DESCRIPTION_DIR}/root/var/tmp
  dprint "Copying ${DESCRIPTION_DIR}/${AFTER_PKGS_INSTALL_FILE} to ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_install_file.json"
  cp ${DESCRIPTION_DIR}/${AFTER_PKGS_INSTALL_FILE} ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_install_file.json
fi

if [[ -n ${AFTER_PKGS_EXECUTE_FILE} && -f "${DESCRIPTION_DIR}/${AFTER_PKGS_EXECUTE_FILE}" ]]; then
  mkdir -p ${DESCRIPTION_DIR}/root/var/tmp
  dprint "Copying ${DESCRIPTION_DIR}/${AFTER_PKGS_EXECUTE_FILE} to ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_execute_file.json"
  cp ${DESCRIPTION_DIR}/${AFTER_PKGS_EXECUTE_FILE} ${DESCRIPTION_DIR}/root/var/tmp/after_pkgs_execute_file.json
fi

# Generate the images
PXE_RC=0
ONIE_RC=0

if [ -n "${BUILD_PXE}" ]; then
  dprint "Generating PXE and USB installer, this will take few minutes..."
  rm -rf ${TARGET_DIR}/btrfs
  (
    set -e -o pipefail
    kiwi-ng-3 \
      --profile FBOSS \
      --type oem \
      ${KIWI_DEBUG} system build \
      --description ${DESCRIPTION_DIR} \
      --target-dir ${TARGET_DIR}/btrfs |&
      stdbuf -oL tee -a ${LOG_FILE} | stdbuf -oL awk '{print "PXE/USB Installer| " $0}'
    mv ${TARGET_DIR}/btrfs/FBOSS-Distro-Image.x86_64-1.0.install.* ${TARGET_DIR}
  ) &
  PXE_PID=$!
fi

if [ -n "${BUILD_ONIE}" ]; then
  dprint "Generating ONIE installer, this will take few minutes..."
  rm -rf ${TARGET_DIR}/onie
  (
    set -e -o pipefail
    kiwi-ng-3 \
      --profile FBOSS \
      --type tbz \
      ${KIWI_DEBUG} system build \
      --description ${DESCRIPTION_DIR} \
      --target-dir ${TARGET_DIR}/onie |&
      stdbuf -oL tee -a ${LOG_FILE} | stdbuf -oL awk '{print "ONIE installer| " $0}'

    # Repack the rootfs so really long filenames are not truncated under Busybox
    dprint "Repacking rootfs with zstd..."
    mkdir ${TARGET_DIR}/onie/rootfs
    pushd ${TARGET_DIR}/onie/rootfs >/dev/null
    xzcat --threads=0 ${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.tar.xz | tar -x
    rm ${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.tar.xz
    tar --format=gnu -cf ${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.tar *
    zstd --threads=0 -19 ${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.tar
    popd >/dev/null

    build_onie_installer | awk '{print "ONIE installer| " $0}'
    mv ${TARGET_DIR}/onie/FBOSS-Distro-Image.x86_64-1.0.install.* ${TARGET_DIR}
  ) &
  ONIE_PID=$!
fi

if [ -n "${BUILD_PXE}" ]; then
  wait ${PXE_PID}
  PXE_RC=$?
fi

if [ -n "${BUILD_ONIE}" ]; then
  wait ${ONIE_PID}
  ONIE_RC=$?
fi

if [ ${PXE_RC} -ne 0 ]; then
  dprint "ERROR: PXE/USB image build failed"
fi
if [ ${ONIE_RC} -ne 0 ]; then
  dprint "ERROR: ONIE installer build failed"
fi

RC=$((PXE_RC + ONIE_RC))
dprint "Image generation completed with exit code ${RC}"
exit "${RC}"
