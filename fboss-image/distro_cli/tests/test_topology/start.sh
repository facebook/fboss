#!/bin/bash
# Start the test topology for fboss-image CLI testing
#
# Starts proxy-device container for integration testing
#
# Usage:
#   ./start.sh        # Start topology
#   ./start.sh stop   # Stop topology
#   ./start.sh status # Show status
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

cleanup_loop_devices() {
  # Clean up stale loop devices from previous container runs
  # (loop devices are shared with host and persist after container exit)
  for dev in $(losetup -j "/var/btrfs.img" 2>/dev/null | cut -d: -f1); do
    if ! sudo losetup -d "$dev" 2>/dev/null; then
      echo "Warning: failed to detach loop device $dev (may need sudo)" >&2
    fi
  done
}

stop_topology() {
  echo "Stopping test topology..."
  docker stop proxy-device 2>/dev/null || true
  docker rm proxy-device 2>/dev/null || true
  cleanup_loop_devices
  echo "Stopped."
}

show_status() {
  echo "=== Test Topology Status ==="
  if docker ps --format '{{.Names}}' | grep -q proxy-device; then
    DEVICE_MAC=$(docker exec proxy-device cat /sys/class/net/eth0/address 2>/dev/null || echo "unknown")
    DEVICE_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' proxy-device 2>/dev/null || echo "unknown")
    echo "proxy-device: running (MAC: ${DEVICE_MAC}, IP: ${DEVICE_IP})"
  else
    echo "proxy-device: not running"
  fi
  echo ""
  echo "=== Service Status ==="
  docker exec proxy-device bash -c 'for svc in wedge_agent fsdb platform_manager; do systemctl is-active $svc 2>/dev/null && echo $svc || true; done' 2>/dev/null || echo "(proxy-device not running)"
}

case "${1:-start}" in
stop)
  stop_topology
  exit 0
  ;;
status)
  show_status
  exit 0
  ;;
start) ;;
*)
  echo "Usage: $0 [start|stop|status]"
  exit 1
  ;;
esac

# Stop any existing containers
stop_topology

# Paths relative to test_topology location
PROXY_DEVICE_DIR="${SCRIPT_DIR}/../proxy_device"

# Ensure proxy_device image is built
echo "Checking container images..."
if ! docker image inspect fboss_proxy_device >/dev/null 2>&1; then
  echo "Building proxy_device..."
  (cd "${PROXY_DEVICE_DIR}" && ./build.sh)
fi

# Clean up stale loop devices before starting
cleanup_loop_devices

# Start proxy device on docker0 bridge
echo "Starting proxy device..."
docker run -d --privileged --cgroupns=host --name proxy-device fboss_proxy_device /sbin/init

# Wait for device to initialize (poll for wedge_agent to be active)
echo "Waiting for device initialization..."
for _ in {1..60}; do
  if docker exec proxy-device systemctl is-active wedge_agent >/dev/null 2>&1; then
    break
  fi
  sleep 0.5
done

# Show status
echo ""
show_status
echo ""
DEVICE_MAC=$(docker exec proxy-device cat /sys/class/net/eth0/address)
DEVICE_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' proxy-device)
echo "=== Device Info ==="
echo "  MAC: ${DEVICE_MAC}"
echo "  IP:  ${DEVICE_IP}"
echo ""
echo "To stop: ./start.sh stop"
