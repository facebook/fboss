---
id: running
title: Running PlatformManager
sidebar_label: Running
sidebar_position: 7
---

# Running PlatformManager

PlatformManager can run in two modes: one-shot mode for initial platform setup,
or daemon mode for continuous operation with a Thrift service.

## Command-Line Flags

### Core Flags

| Flag                   | Default | Description                                                                              |
| ---------------------- | ------- | ---------------------------------------------------------------------------------------- |
| `--run_once`           | `true`  | If true, explore platform once and exit. If false, run as a daemon with Thrift service. |
| `--thrift_port`        | `5975`  | Port for the Thrift service (only used when `--run_once=false`)                          |
| `--explore_interval_s` | `60`    | Interval between exploration cycles in daemon mode                                       |

### Package Management Flags

| Flag                 | Default | Description                                                            |
| -------------------- | ------- | ---------------------------------------------------------------------- |
| `--enable_pkg_mgmnt` | `true`  | Enable BSP package management (RPM download and installation)          |
| `--reload_kmods`     | `false` | Force reload of kernel modules even if already loaded                  |
| `--local_rpm_path`   | `""`    | Path to a local RPM file for testing (bypasses normal RPM resolution)  |

## Running Modes

### One-Shot Mode (Default)

```bash
platform_manager --run_once=true
```

This mode:
1. Loads the platform configuration
2. Processes BSP packages (installs RPMs, loads kmods)
3. Explores the platform and creates device symlinks
4. Exits with status 0 on success, 1 on failure


### Daemon Mode

```bash
platform_manager --run_once=false
```

Performs the same steps as one-shot mode, but instead of exiting:
1. Starts the Thrift service on the configured port
2. Continues running to handle API requests
3. Sends `sd_notify` ready signal for systemd integration
