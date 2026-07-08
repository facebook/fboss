#!/bin/bash
VERSION="1.0.0"
SERVICE_NAME=$(basename "$0")
VERSION_FILE="/var/run/${SERVICE_NAME}.version"
LOG_FILE="/var/log/${SERVICE_NAME}.log"

mkdir -p /var/run /var/log
echo "$VERSION" >"$VERSION_FILE"
echo "$(date): $SERVICE_NAME v$VERSION started (pid $$)" >>"$LOG_FILE"

while true; do sleep 60; done
