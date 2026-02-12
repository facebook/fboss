#!/bin/bash
# Initialize FBOSS configuration based on platform detection

set -e

FBOSS_SHARE="/opt/fboss/share"
COOP_DIR="/etc/coop"
FRUID_FILE="/var/facebook/fboss/fruid.json"

log() {
  echo "[fboss_init] $1" >&2
}

error() {
  echo "[fboss_init] ERROR: $1" >&2
}

get_platform_dir() {
  local platform
  # convert the platform name to lowercase and delete spaces
  platform=$(dmidecode -s system-product-name 2>/dev/null | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]-_')
  if [[ -z $platform ]]; then
    error "Failed to get system-product-name from dmidecode"
    return 1
  fi
  log "Detected platform: $platform"

  local platform_dir="${FBOSS_SHARE}/default_configs/${platform}"
  if [[ ! -d $platform_dir ]]; then
    error "Platform config directory not found: $platform_dir"
    return 1
  fi
  log "Using platform config directory: $platform_dir"

  echo "$platform_dir"
}

copy_config() {
  local src="$1"
  local dst="$2"
  local name="$3"

  if [[ -e $dst ]]; then
    log "$name already exists at $dst (skipping)"
    return
  fi

  if [[ -f $src ]]; then
    cp "$src" "$dst"
    log "Copied $name: $src -> $dst"
  else
    log "No $name found at $src (skipping)"
  fi
}

generate_fruid() {
  if [[ -e $FRUID_FILE ]]; then
    log "fruid.json already exists at $FRUID_FILE (skipping)"
    return
  fi

  mkdir -p "$(dirname "$FRUID_FILE")"

  if /opt/fboss/bin/weutil --json >"$FRUID_FILE" 2>/dev/null; then
    log "Generated fruid.json: $FRUID_FILE"
  else
    error "Failed to generate fruid.json"
    rm -f "$FRUID_FILE"
  fi
}

setup_coop_configs() {
  local platform_dir="$1"
  mkdir -p "$COOP_DIR"
  copy_config "${platform_dir}/agent.conf" "${COOP_DIR}/agent.conf" "agent.conf"
  copy_config "${platform_dir}/qsfp.conf" "${COOP_DIR}/qsfp.conf" "qsfp.conf"
}

main() {
  log "Starting FBOSS initialization"

  local platform_dir
  if ! platform_dir=$(get_platform_dir); then
    exit 1
  fi

  setup_coop_configs "$platform_dir"
  generate_fruid

  log "FBOSS initialization complete"
}

main "$@"
