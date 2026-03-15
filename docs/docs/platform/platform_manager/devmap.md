---
id: devmap
title: Device Map (/run/devmap)
sidebar_label: Device Map
sidebar_position: 6
---

# PlatformManager Device Map (/run/devmap)

PlatformManager creates symbolic links under `/run/devmap/` to provide stable,
human-readable paths to platform devices. These symlinks should be used
instead of raw device paths like `/dev/i2c-5`.

## Directory Structure

```
/run/devmap/
├── i2c-busses/
│   ├── OPTICS_1 -> /dev/i2c-5
│   ├── OPTICS_2 -> /dev/i2c-6
│   └── SMB_SENSOR_BUS -> /dev/i2c-12
├── gpiochips/
│   ├── SCM_CPLD_GPIO -> /sys/class/gpio/gpiochip432
│   └── SMB_FPGA_GPIO -> /sys/class/gpio/gpiochip400
├── sensors/
│   ├── SCM_INLET_TEMP -> /sys/bus/i2c/devices/5-0048/hwmon/hwmon3
│   └── SMB_OUTLET_TEMP -> /sys/bus/i2c/devices/12-0049/hwmon/hwmon5
├── eeproms/
│   ├── CHASSIS_EEPROM -> /sys/bus/i2c/devices/2-0050/eeprom
│   └── SCM_EEPROM -> /sys/bus/i2c/devices/5-0051/eeprom
├── watchdogs/
│   └── FAN_WATCHDOG -> /dev/watchdog0
├── fpgas/
│   └── IOB_FPGA -> /sys/bus/pci/devices/0000:07:00.0
├── cplds/
│   └── SCM_CPLD -> /sys/bus/i2c/devices/5-0035
├── spi-devices/
│   └── BIOS_SPI -> /dev/spidev0.0
├── mdio-busses/
│   └── RTM_MDIO_0 -> /sys/class/mdio_bus/mdio-0
├── xcvrs/
│   └── XCVR_1 -> /sys/bus/pci/devices/.../xcvr_ctrl.1
└── leds/
    └── PORT_1_LED_1 -> /sys/class/leds/port1:led1
```

## Supported Device Groups

| Group         | Description                  | Example Target                           |
| ------------- | ---------------------------- | ---------------------------------------- |
| `i2c-busses`  | I2C bus character devices    | `/dev/i2c-N`                             |
| `gpiochips`   | GPIO controller sysfs paths  | `/sys/class/gpio/gpiochipN`              |
| `sensors`     | Hardware monitoring sensors  | `/sys/bus/i2c/devices/.../hwmon/hwmonN`  |
| `eeproms`     | EEPROM sysfs paths           | `/sys/bus/i2c/devices/.../eeprom`        |
| `watchdogs`   | Watchdog character devices   | `/dev/watchdogN`                         |
| `fpgas`       | FPGA PCI device sysfs paths  | `/sys/bus/pci/devices/...`               |
| `cplds`       | CPLD I2C device sysfs paths  | `/sys/bus/i2c/devices/...`               |
| `spi-devices` | SPI character devices        | `/dev/spidevN.M`                         |
| `mdio-busses` | MDIO bus sysfs paths         | `/sys/class/mdio_bus/mdio-N`             |
| `xcvrs`       | Transceiver controller paths | Vendor-specific                          |
| `leds`        | LED class device paths       | `/sys/class/leds/...`                    |

## Configuration

Symlinks are configured in the platform configuration using
`symbolicLinkToDevicePath`:

```json
{
  "symbolicLinkToDevicePath": {
    "/run/devmap/eeproms/CHASSIS_EEPROM": "/[CHASSIS_EEPROM]",
    "/run/devmap/sensors/SMB_INLET_SENSOR": "/SMB_SLOT@0/[INLET_SENSOR]",
    "/run/devmap/sensors/FAN_CTRL": "/SMB_SLOT@0/[FAN_CTRL]"
  }
}
```

## Naming Conventions

Symlink names should:
- Use UPPER_CASE with underscores
- Be descriptive of the device function
- Include numeric suffixes for multiple instances (e.g., `OPTICS_1`, `OPTICS_2`)
