# Proxy Device

A Docker container that simulates a FBOSS device for testing the `fboss-image` CLI commands, particularly the `device update` functionality.

## Purpose

The `device update` command creates btrfs subvolumes, installs artifacts, and restarts services. Testing this against real hardware is slow and impractical during development.

This container provides a lightweight simulation that:
- Runs systemd as init (like a real device)
- Creates a btrfs filesystem on a loopback file
- Runs proxy FBOSS services in per-service btrfs subvolumes
- Accepts SSH connections for CLI commands

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  proxy_device                                               │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  systemd (PID 1)                                    │    │
│  │                                                     │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │    │
│  │  │ wedge_agent │  │    fsdb     │  │ qsfp_service│  │    │
│  │  │ (subvol)    │  │ (subvol)    │  │ (subvol)    │  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │    │
│  │                                                     │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │    │
│  │  │platform_mgr │  │ sensor_svc  │  │ fan_service │  │    │
│  │  │ (subvol)    │  │ (subvol)    │  │ (subvol)    │  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │    │
│  │                                                     │    │
│  │  ┌─────────────┐                                    │    │
│  │  │data_corral  │        sshd (port 22)              │    │
│  │  │ (subvol)    │                                    │    │
│  │  └─────────────┘                                    │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                             │
│  /mnt/btrfs/                                                │
│  ├── distro-base/          (base subvolume)                 │
│  └── updates/                                               │
│      ├── wedge_agent-<ts>/  (service subvolume)             │
│      ├── fsdb-<ts>/                                         │
│      ├── qsfp_service-<ts>/                                 │
│      └── ...                                                │
└─────────────────────────────────────────────────────────────┘
```

### Directory Structure Within Subvolumes

Each btrfs subvolume contains the FBOSS directory structure:

```
/opt/fboss/
├── bin/
│   └── <service>       # Service script (replaced by update)
└── lib/                # Shared libraries
```

- **Service scripts** (`/opt/fboss/bin/`) are stub scripts that write their version to `/var/run/<service>.version` and loop forever.
- Updates replace these scripts with new versions.
- Tests verify updates by checking the version file contents.

## Key Components

### Dockerfile
Builds a CentOS Stream 9 image with:
- systemd as init
- btrfs-progs for filesystem operations
- SSH server with passwordless root access
- FBOSS service scripts at `/opt/fboss/bin/`

### parts/setup_btrfs.sh
Runs at first boot via `device-init.service`:
1. Creates a 512MB loopback file at `/var/btrfs.img`
2. Formats it as btrfs and mounts at `/mnt/btrfs`
3. Creates `distro-base` subvolume with FBOSS directory structure
4. Snapshots base into per-service subvolumes under `/updates/`
5. Creates systemd drop-ins setting `RootDirectory=` for each service

### parts/services/<service>
Each service script (wedge_agent, fsdb, etc.) is a simple stub that:
1. Writes its VERSION to `/var/run/<service>.version`
2. Logs startup to `/var/log/<service>.log`
3. Loops forever with `sleep 60`

The initial version is "1.0.0". Updates replace the script with a new version.

### Version Verification

Each service writes its version to `/var/run/<service>.version` **inside its btrfs subvolume**.

To verify from outside the service:
```bash
# Find the service's subvolume and check version
SUBVOL=$(ls -d /mnt/btrfs/updates/wedge_agent-* | head -1)
cat $SUBVOL/var/run/wedge_agent.version
# Output: 1.0.0 (before update) or 2.0.0 (after update)
```

Integration tests use this flow:
1. Start container → services run with VERSION="1.0.0"
2. Deploy update with VERSION="2.0.0" artifacts
3. Restart service
4. Verify version file changed to "2.0.0"

## Usage

```bash
# Build the container image
./build.sh

# Run standalone (for debugging)
docker run -d --privileged --cgroupns=host --name proxy-device \
    fboss_proxy_device /sbin/init

# SSH into it (passwordless root login)
ssh root@<container-ip>

# Check services
docker exec proxy-device systemctl status wedge_agent

# Check subvolumes
docker exec proxy-device ls /mnt/btrfs/updates/

# Check service version
docker exec proxy-device bash -c 'cat /mnt/btrfs/updates/wedge_agent-*/var/run/wedge_agent.version'
```

## Testing Updates

The `update` command replaces service scripts and restarts services.

**How it works:**

1. Service scripts start with VERSION="1.0.0" at `/opt/fboss/bin/<service>`
2. Systemd runs these scripts: `ExecStart=/opt/fboss/bin/<service>`
3. Updates replace scripts with new versions (e.g., VERSION="2.0.0")
4. Service restarts and writes new version to `/var/run/<service>.version`

Tests verify updates by checking:
- Version file contains expected version
- Service is running (via systemctl status)
- Log file shows new startup entry

## Requirements

- Docker with `--privileged` support (for systemd and loopback mounts)
- `--cgroupns=host` for proper cgroup management
