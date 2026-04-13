# Test Topology

A test environment that provides a test device for end-to-end testing of `fboss-image` CLI device commands.

## Purpose

The `fboss-image` CLI device commands (`ssh`, `update`) require a target device to connect to via SSH.

This topology provides a test device container, allowing integration tests to run against a real containerized device rather than mocks.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Host (docker0 bridge: 172.17.0.0/16)                           │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │  proxy-device       │                                        │
│  │  (docker0 bridge)   │                                        │
│  │                     │                                        │
│  │  - systemd          │                                        │
│  │  - btrfs subvolumes │                                        │
│  │  - FBOSS services   │                                        │
│  │  - sshd             │                                        │
│  └─────────────────────┘                                        │
│         ▲                                                       │
│         │                                                       │
│         │ SSH / docker exec                                     │
│         │                                                       │
│  ┌──────┴──────────────────────────────┐                        │
│  │  fboss-image CLI                    │                        │
│  │                                     │                        │
│  │  ./fboss-image device <IP> ssh      │                        │
│  │  ./fboss-image device <IP> update   │                        │
│  └─────────────────────────────────────┘                        │
└─────────────────────────────────────────────────────────────────┘
```

## Usage

### Starting the Topology

```bash
# Start the test device
./start.sh

# Output shows device MAC and IP:
#   MAC: 02:42:ac:11:00:02
#   IP:  172.17.0.2
```

### Checking Status

```bash
./start.sh status
```

### Stopping the Topology

```bash
./start.sh stop
```

## What start.sh Does

1. Stops any existing `proxy-device` container
2. Builds the `fboss_proxy_device` image if not present
3. Cleans up stale loop devices from previous runs
4. Starts the test device on the docker0 bridge network with systemd
5. Waits for device initialization (polls for `wedge_agent` to be active)
6. Displays device MAC, IP, and service status

## Integration Tests

The `device_integration_test.py` uses this topology:

```bash
# Start topology (optional - tests will start if needed)
./start.sh

# Run integration tests
cd ../..  # distro_cli directory
python3 -m pytest tests/device_integration_test.py -v

# Tests reuse running topology for speed
# Topology is only stopped if tests started it
```

## Testing Updates with FBOSS Services

The `update` command implementation is the same regardless of target. The test device runs FBOSS service scripts that write version files for verification.

**How it works:**

1. Base container runs services with `VERSION="1.0.0"` at `/opt/fboss/bin/<service>`
2. Each service writes its version to `/var/run/<service>.version` on startup
3. Updates deploy new service scripts with `VERSION="2.0.0"`
4. After service restart, the version file reflects the new version

**Tests verify** the version changes from 1.0.0 to 2.0.0 after an update, confirming the full update workflow including btrfs subvolume creation and service restart.

See `proxy_device/README.md` for details on the test device architecture.

## Files

- `start.sh` - Topology management script (start/stop/status)
- `../proxy_device/` - Test device container definition

## Network Details

- Test device runs on docker0 bridge (typically 172.17.0.x)
- Device is accessible via its docker-assigned IP address
- SSH is available on port 22
- CLI commands use the device IP directly (no MAC-to-IP resolution needed)

## Troubleshooting

**SSH connection refused:**
- Wait for device initialization (~30 seconds after start)
- Check sshd is running: `docker exec proxy-device systemctl status sshd`
- Verify device IP: `docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' proxy-device`

**Services not running:**
- Check device-init completed: `docker exec proxy-device systemctl status device-init`
- View logs: `docker exec proxy-device journalctl -u device-init`
- Check service status: `./start.sh status`
