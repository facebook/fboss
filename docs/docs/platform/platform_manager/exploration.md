---
id: exploration
title: Exploration Process
sidebar_label: Exploration
sidebar_position: 5
---

# PlatformManager Exploration

Platform exploration is the process of discovering and setting up all devices
on the platform based on the configuration file.

## Exploration Workflow

1. **Load Configuration**: Read platform-specific JSON config
2. **Package Management**: Install BSP RPM and load kernel modules
3. **Discover Root PmUnit**: Start from the root PmUnit (typically SCM)
4. **Recursive Exploration**:
   - Read IDPROM to identify PmUnit type
   - Create PCI sub-devices (from FPGA)
   - Create I2C devices
   - Create symbolic links in `/run/devmap/`
   - Explore child slots recursively
5. **Publish Versions**: Read and publish firmware/hardware versions
6. **Report Status**: Log results and publish ODS counters

## Exploration Status

After exploration, the status is one of:

| Status                           | Description                                                  |
| -------------------------------- | ------------------------------------------------------------ |
| `SUCCEEDED`                      | All devices explored without errors                          |
| `SUCCEEDED_WITH_EXPECTED_ERRORS` | Completed with expected errors (e.g., empty optional slots)  |
| `FAILED`                         | Exploration encountered unexpected errors                    |
| `IN_PROGRESS`                    | Exploration is currently running                             |
| `UNSTARTED`                      | Exploration has not started                                  |

## Error Types

Exploration errors are categorized by type:

| Error Type                | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| `I2C_DEVICE_CREATE`       | Failed to create an I2C device                                    |
| `I2C_DEVICE_REG_INIT`     | Failed to initialize I2C device registers                         |
| `I2C_DEVICE_EXPLORE`      | Failed during I2C device exploration                              |
| `RUN_DEVMAP_SYMLINK`      | Failed to create symlink in /run/devmap                           |
| `PCI_DEVICE_EXPLORE`      | Failed to explore PCI device                                      |
| `PCI_SUB_DEVICE_CREATE_*` | Failed to create FPGA sub-device (I2C adapter, SPI, GPIO, etc.)   |
| `IDPROM_READ`             | Failed to read IDPROM                                             |
| `SLOT_PM_UNIT_ABSENCE`    | PmUnit not present in slot                                        |
| `SLOT_PRESENCE_CHECK`     | Failed to check slot presence                                     |


## Querying Exploration Results

### Via Thrift API

Other FBOSS services can query exploration results programmatically using the
[Service API](/docs/platform/platform_manager/service_api):

- `getLastPMStatus()` - Returns exploration status and failed devices map
- `getAllPmUnits()` - Returns all discovered PmUnits with their status

### Via Logs

Exploration results are logged at INFO level:

```
Successfully explored <platform_name>...
```

Errors are logged at WARNING or ERROR level with device paths and error
messages.
