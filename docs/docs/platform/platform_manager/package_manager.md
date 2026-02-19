---
id: package_manager
title: Package Manager
sidebar_label: Package Manager
sidebar_position: 8
---

# PlatformManager Package Manager

The Package Manager component of PlatformManager handles BSP (Board Support
Package) RPM management and kernel module loading.

## Overview

Before platform exploration can occur, the correct kernel modules must be
loaded. The Package Manager:

1. Installs the BSP RPM if not already installed
2. Unloads any existing BSP kernel modules
3. Loads required kernel modules in the correct order
4. Handles watchdog devices during kmod operations

## BSP RPM Configuration

The platform configuration specifies the BSP RPM:

```json
{
  "bspKmodsRpmName": "vendor_bsp_kmods",
  "bspKmodsRpmVersion": "1.0.0",
  "requiredKmodsToLoad": ["module1", "module2"]
}
```

- `bspKmodsRpmName`: Base name of the RPM package
- `bspKmodsRpmVersion`: Required version
- `requiredKmodsToLoad`: Modules to load before exploration (most modules are
  loaded automatically during device creation)

## Kernel Modules Management

BSP vendors provide a `kmods.json` file as part of BSP RPM that lists the kernel modules. This file is located at `/usr/local/{vendor}_bsp/kmods.json` and is used by PlatformManager to unload the kernel modules.

```json
{
  "bspKmods": [
    "vendor_fpga_driver",
    "vendor_i2c_driver",
    "vendor_fan_driver"
  ],
  "sharedKmods": [
    "i2c_mux_pca954x",
    "at24"
  ]
}
```

When `--reload_kmods=true` or when the BSP version changes:

1. Close any open watchdog devices (prevents system reset)
2. Unload `bspKmods` in reverse order
3. Unload `sharedKmods` in reverse order
4. Run `depmod` to refresh module dependencies
5. Load `requiredKmodsToLoad` modules
6. Proceed with platform exploration (which loads modules on-demand)

## Local RPM Testing

For development, a local RPM can be tested by specifying its path:

```bash
platform_manager --local_rpm_path=/path/to/<vendor>_bsp_kmods-<version>.rpm
```

This bypasses normal RPM resolution and installs the specified file directly.

Note: The actual BSP RPM name usually contains version information like `<vendor>_bsp_kmods-<kernel_version>.x86_64-<bsp_version>-1`. For example, `fboss_bsp_kmods-6.11.1-1.fboss.el9.x86_64-4.0.0-1`.


## Disabling Package Management

Package management can be skipped for debugging purposes:

```bash
platform_manager --enable_pkg_mgmnt=false
```

This assumes all required modules are already loaded.
