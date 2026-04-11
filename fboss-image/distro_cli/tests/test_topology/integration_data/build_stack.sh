#!/bin/bash
# Build update tarball for integration tests with VERSION="2.0.0" service scripts.
set -euo pipefail

STACK_NAME="$1"
OUTPUT_DIR="${2:-/output}"

case "$STACK_NAME" in
forwarding-stack) SERVICES="wedge_agent fsdb qsfp_service" ;;
platform-stack) SERVICES="platform_manager sensor_service fan_service data_corral_service" ;;
*)
  echo "Unknown stack: $STACK_NAME" >&2
  exit 1
  ;;
esac

# Service script template with VERSION="2.0.0"
SERVICE_SCRIPT='#!/bin/bash
VERSION="2.0.0"
SERVICE_NAME=$(basename "$0")
VERSION_FILE="/var/run/${SERVICE_NAME}.version"
LOG_FILE="/var/log/${SERVICE_NAME}.log"

mkdir -p /var/run /var/log
echo "$VERSION" >"$VERSION_FILE"
echo "$(date): $SERVICE_NAME v$VERSION started (pid $$)" >>"$LOG_FILE"

while true; do sleep 60; done
'

STAGING=$(mktemp -d)
trap 'rm -rf "$STAGING"' EXIT
mkdir -p "$STAGING/bin"

for svc in $SERVICES; do
  echo "$SERVICE_SCRIPT" >"$STAGING/bin/$svc"
  chmod +x "$STAGING/bin/$svc"
done

tar -cf "$OUTPUT_DIR/$STACK_NAME.tar" -C "$STAGING" bin
