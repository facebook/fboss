#!/bin/bash
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

# Update script that runs on the FBOSS device to update services.
# Installs whatever artifact exists in the script's directory.
# Usage: ./update_service.sh <component> <service1> [service2] ...

set -eou pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <component> [service1] [service2] ..." >&2
  exit 1
fi

COMPONENT="$1"
shift

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

SERVICES="$*"
if [ -z "${SERVICES}" ]; then
  echo "Error: Component '${COMPONENT}' requires at least one service" >&2
  echo "Usage: $0 <component> <service1> [service2] ..." >&2
  exit 1
fi

TIMESTAMP=$(date +%s)
# Resolve symlinks to real paths (needed for systemd RootDirectory which doesn't follow symlinks)
BASE_SNAPSHOT="$(readlink -f /distro-base)"
UPDATES_DIR="$(readlink -f /updates)"

echo "Updating component: ${COMPONENT}"
echo "Services to restart: ${SERVICES}"

# Extract all artifacts from script's directory to staging
mkdir -p "${UPDATES_DIR}"
STAGING_DIR=$(mktemp -d -p "${UPDATES_DIR}")
trap 'rm -rf "${STAGING_DIR}"' EXIT

FOUND_ARTIFACT=false
shopt -s nullglob
for f in "${SCRIPT_DIR}"/*.tar.zst "${SCRIPT_DIR}"/*.tar; do
  FOUND_ARTIFACT=true
  echo "Extracting ${f}..."
  tar -xf "$f" -C "${STAGING_DIR}"
done
shopt -u nullglob

if [ "${FOUND_ARTIFACT}" = false ]; then
  echo "Error: No artifact found in ${SCRIPT_DIR}" >&2
  exit 1
fi

for svc in ${SERVICES}; do
  SNAPSHOT_PATH="${UPDATES_DIR}/${svc}-${TIMESTAMP}"
  echo "Creating snapshot for ${svc}: ${SNAPSHOT_PATH}"

  btrfs subvolume snapshot "${BASE_SNAPSHOT}" "${SNAPSHOT_PATH}"
  cp -a "${STAGING_DIR}"/* "${SNAPSHOT_PATH}/opt/fboss/"

  mkdir -p "/etc/systemd/system/${svc}.service.d/"
  cat >"/etc/systemd/system/${svc}.service.d/root-override.conf" <<EOF
[Service]
RootDirectory=${SNAPSHOT_PATH}
EOF
done

echo "Reloading systemd and restarting services..."
systemctl daemon-reload
for svc in ${SERVICES}; do
  echo "  Restarting ${svc}..."
  systemctl restart "${svc}"
done

# Delete old subvolumes after restart succeeds
for svc in ${SERVICES}; do
  SNAPSHOT_PATH="${UPDATES_DIR}/${svc}-${TIMESTAMP}"
  for old in "${UPDATES_DIR}/${svc}"-*; do
    if [ -d "$old" ] && [ "$old" != "${SNAPSHOT_PATH}" ]; then
      echo "  Deleting old snapshot: ${old}"
      btrfs subvolume delete "$old" 2>/dev/null || true
    fi
  done
done

echo "Update complete for component: ${COMPONENT}"
