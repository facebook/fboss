---
id: block_configs
title: Block Configurations
sidebar_label: Block Configs
sidebar_position: 3
---

# PlatformManager Block Configuration

Block configurations allow defining multiple similar FPGA sub-devices using
offset calculation expressions. This reduces configuration verbosity when an
FPGA has many identical controllers at regular address offsets.

## Overview

Instead of listing each I2C adapter, LED controller, or transceiver controller
individually, block configs define a pattern with:
- A name prefix
- An offset calculation expression
- The number of instances

## Offset Calculation Expressions

Expressions support:
- Hexadecimal constants: `0x1000`
- Arithmetic: `+`, `-`, `*`
- Variables in braces (context-dependent, e.g., `{adapterIndex}`, `{busIndex}`)

Available variables depend on the block config type:
- **I2C adapter blocks**: `{adapterIndex}`, `{startAdapterIndex}`
- **MDIO bus blocks**: `{busIndex}`
- **Port/LED configs**: `{portNum}`, `{startPort}`, `{ledNum}`

The result is computed as a hexadecimal offset.

### Example Calculation

For `csrOffsetCalc: "0x1000 + ({adapterIndex} - {startAdapterIndex}) * 0x100"`
with `startAdapterIndex=1`:

| adapterIndex | Calculation          | Result |
|--------------|----------------------|--------|
| 1            | 0x1000 + (1-1)*0x100 | 0x1000 |
| 2            | 0x1000 + (2-1)*0x100 | 0x1100 |
| 3            | 0x1000 + (3-1)*0x100 | 0x1200 |


## I2C Adapter Block Config

Creates multiple I2C adapters from an FPGA:

```json
{
  "i2cAdapterBlockConfigs": [
    {
      "pmUnitScopedNamePrefix": "SMB_I2C_ADAPTER",
      "deviceName": "fbiob_i2c",
      "csrOffsetCalc": "0x1000 + ({adapterIndex} - {startAdapterIndex}) * 0x100",
      "iobufOffsetCalc": "0x2000 + ({adapterIndex} - {startAdapterIndex}) * 0x100",
      "startAdapterIndex": 1,
      "numAdapters": 8,
      "numBusesPerAdapter": 1
    }
  ]
}
```

This creates devices named `SMB_I2C_ADAPTER_1` through `SMB_I2C_ADAPTER_8`.

| Field                    | Description                        |
|--------------------------|------------------------------------|
| `pmUnitScopedNamePrefix` | Prefix for generated device names  |
| `deviceName`             | Kernel driver name                 |
| `csrOffsetCalc`          | Expression for CSR register offset |
| `iobufOffsetCalc`        | Expression for I/O buffer offset   |
| `startAdapterIndex`      | Starting index for numbering       |
| `numAdapters`            | Number of adapters to create       |
| `numBusesPerAdapter`     | I2C buses per adapter (default: 1) |

**Available variables:**

| Variable              | Description                                                                               |
|-----------------------|-------------------------------------------------------------------------------------------|
| `{adapterIndex}`      | Current adapter index (from `startAdapterIndex` to `startAdapterIndex + numAdapters - 1`) |
| `{startAdapterIndex}` | Value of `startAdapterIndex` field                                                        |

## Transceiver Controller Block Config

Creates multiple transceiver (QSFP/SFP) controllers:

```json
{
  "xcvrCtrlBlockConfigs": [
    {
      "pmUnitScopedNamePrefix": "XCVR_CTRL",
      "deviceName": "fbiob_xcvr",
      "csrOffsetCalc": "0x3000 + ({portNum} - {startPort}) * 0x4",
      "iobufOffsetCalc": "0x4000 + ({portNum} - {startPort}) * 0x4",
      "startPort": 1,
      "numPorts": 32
    }
  ]
}
```

This creates devices named `XCVR_CTRL_PORT_1` through `XCVR_CTRL_PORT_32`.

| Field                    | Description                         |
|--------------------------|-------------------------------------|
| `pmUnitScopedNamePrefix` | Prefix for generated device names   |
| `deviceName`             | Kernel driver name                  |
| `csrOffsetCalc`          | Expression for CSR register offset  |
| `iobufOffsetCalc`        | Expression for I/O buffer offset    |
| `startPort`              | Starting port number                |
| `numPorts`               | Number of ports                     |

**Available variables:**

| Variable      | Description                                                          |
|---------------|----------------------------------------------------------------------|
| `{portNum}`   | Current port number (from `startPort` to `startPort + numPorts - 1`) |
| `{startPort}` | Value of `startPort` field                                           |

## LED Controller Block Config

Creates multiple LED controllers (typically per-port):

```json
{
  "ledCtrlBlockConfigs": [
    {
      "pmUnitScopedNamePrefix": "LED_CTRL",
      "deviceName": "fbiob_led",
      "csrOffsetCalc": "0x5000 + ({portNum} - {startPort}) * 0x8 + ({ledNum} - 1) * 0x4",
      "iobufOffsetCalc": "0x6000 + ({portNum} - {startPort}) * 0x8 + ({ledNum} - 1) * 0x4",
      "startPort": 1,
      "numPorts": 32,
      "ledPerPort": 2
    }
  ]
}
```

This creates devices named `LED_CTRL_PORT_1_LED_1`, `LED_CTRL_PORT_1_LED_2`,
through `LED_CTRL_PORT_32_LED_2`.

| Field                    | Description                        |
|--------------------------|----------------------------------- |
| `pmUnitScopedNamePrefix` | Prefix for generated device names  |
| `deviceName`             | Kernel driver name                 |
| `csrOffsetCalc`          | Expression for CSR register offset |
| `iobufOffsetCalc`        | Expression for I/O buffer offset   |
| `startPort`              | Starting port number               |
| `numPorts`               | Number of ports                    |
| `ledPerPort`             | LEDs per port                      |

**Available variables:**

| Variable      | Description                                                          |
|---------------|----------------------------------------------------------------------|
| `{portNum}`   | Current port number (from `startPort` to `startPort + numPorts - 1`) |
| `{startPort}` | Value of `startPort` field                                           |
| `{ledNum}`    | Current LED number (from 1 to `ledPerPort`)                          |

## MDIO Bus Block Config

Creates multiple MDIO bus controllers:

```json
{
  "mdioBusBlockConfigs": [
    {
      "pmUnitScopedNamePrefix": "RTM_MDIO_BUS",
      "deviceName": "fbiob_mdio",
      "csrOffsetCalc": "0x200 + {busIndex} * 0x20",
      "iobufOffsetCalc": "0x300 + {busIndex} * 0x4",
      "numBuses": 4
    }
  ]
}
```

This creates devices named `RTM_MDIO_BUS_0` through `RTM_MDIO_BUS_3`.

| Field                    | Description                        |
|--------------------------|----------------------------------- |
| `pmUnitScopedNamePrefix` | Prefix for generated device names  |
| `deviceName`             | Kernel driver name                 |
| `csrOffsetCalc`          | Expression for CSR register offset |
| `iobufOffsetCalc`        | Expression for I/O buffer offset   |
| `numBuses`               | Number of MDIO buses               |

**Available variables:**

| Variable     | Description                                  |
|--------------|----------------------------------------------|
| `{busIndex}` | Current bus index (from 0 to `numBuses - 1`) |


## Legacy Configs

Block configs replace the deprecated individual configs:
- `i2cAdapterConfigs` -> `i2cAdapterBlockConfigs`
- `xcvrCtrlConfigs` -> `xcvrCtrlBlockConfigs`
- `ledCtrlConfigs` -> `ledCtrlBlockConfigs`
- `mdioBusConfigs` -> `mdioBusBlockConfigs`

The legacy configs are still supported but should not be used for new platforms.
