# FBOSS BSP Kernel Module API Specification

### Version 1.0.0


- [FBOSS BSP Kernel Module API Specification](#fboss-bsp-kernel-module-api-specification)
    - [Version 1.0.0](#version-100)
- [1. Device References](#1-device-references)
- [2. PCIe Character Device Interface](#2-pcie-character-device-interface)
  - [2.1 Subdevice (I/O Controller) Creation \& Deletion](#21-subdevice-io-controller-creation--deletion)
  - [2.2 Supported Controller Interfaces](#22-supported-controller-interfaces)
    - [2.2.1 FPGA Information (fpga\_info)](#221-fpga-information-fpga_info)
    - [2.2.2 I2C Controller (i2c\_master)](#222-i2c-controller-i2c_master)
    - [2.2.3 SPI Controller (spi\_master)](#223-spi-controller-spi_master)
    - [2.2.4 GPIO Controller (gpiochip)](#224-gpio-controller-gpiochip)
    - [2.2.5 HWMON and PMBUS Device](#225-hwmon-and-pmbus-device)
    - [2.2.6 Fan Controller (fan\_ctrl)](#226-fan-controller-fan_ctrl)
    - [2.2.7 Fan Watchdog (watchdog\_fan)](#227-fan-watchdog-watchdog_fan)
    - [2.2.8 Transceiver Controller (xcvr\_ctrl)](#228-transceiver-controller-xcvr_ctrl)
    - [2.2.9 LED](#229-led)


# 1. Device References

Any interaction between userspace and devices shall use the device interfaces
created by the FBOSS PlatformManager service.

# 2. PCIe Character Device Interface

Most I/O (I2C, SPI, etc) Controllers are provided by the PCIe FPGA in the FBOSS
environment.

## 2.1 Subdevice (I/O Controller) Creation & Deletion

Device topology/settings are managed from userspace with the PlatformManager
service.

The PCIe FPGA driver registers a character device for each FPGA instance in the
system with the following naming pattern:

`fbiob_%04x.%04x.%04x.%04x% (vendor, device, subsystem_vendor, subsystem_device)`

This character device supports the following IOCTL commands:

* `FBIOB_IOC_NEW_DEVICE`
* `FBIOB_IOC_DEL_DEVICE`

More details can be found in the [fbiob-ioctl.h](https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/uapi/fbiob-ioctl.h)
header file.

## 2.2 Supported Controller Interfaces

### 2.2.1 FPGA Information (fpga_info)

**Interface:**

The FPGA Info driver exports the following sysfs files:

* `fw_ver`
  * **Type**: unsigned integer
  * **Description**: This file reports the FPGA's firmware version in the format `"%u.%u\n", major_ver, minor_ver`
  * **Read/Write**: RO


### 2.2.2 I2C Controller (i2c_master)

**Interface:**

Below are the list of I2C sysfs and character device interfaces used by FBOSS
user space services:

* `/sys/bus/i2c/drivers/`
* `/sys/bus/i2c/devices/i2c-#/`
* `/sys/bus/i2c/devices/<bus>-00<addr>/`
* `/dev/i2c-#`

PlatformManager creates mappings between `/run/devmap/i2c-busses/BUS_NAME` and
the corresponding character device `/dev/i2c-#`. For example:

* `/run/devmap/i2c-busses/OPTICS_1` → `/dev/i2c-5`

Once an i2c adapter has been created, devices are instantiated by using
userspace API as described here:

* [docs.kernel.org/i2c/instantiating-devices](https://docs.kernel.org/i2c/instantiating-devices.html#method-4-instantiate-from-user-space)

Example:

    # echo eeprom 0x50 > /sys/bus/i2c/devices/i2c-3/new_device

If devices attached to this adapter are not managed by a kernel driver it is
possible to interact with them directly by using `ioctl` commands. See the below
document for a full explanation.
* [kernel.org/doc/Documentation/i2c/dev-interface](https://www.kernel.org/doc/Documentation/i2c/dev-interface)

### 2.2.3 SPI Controller (spi_master)

**Interface:**

FBOSS user space services don’t access SPI busses directly, instead they operate
on SPI clients. API specification is described in the following document.
[kernel.org/doc/html/latest/spi/spi-summary](https://www.kernel.org/doc/html/latest/spi/spi-summary.html#how-do-these-driver-programming-interfaces-work)

**SPI EEPROMs**
* Flashes and EEPROM devices must be managed by kernel drivers, and user space
  programs access devices through their client driver interfaces. Please see the
  documentation for the eeprom driver interface defiition.

**SPIDEV**
*  Such devices are accessed from user space via the character device
  `/dev/spidev[bus].[cs]`. Refer to below URL for details:

    [kernel.org/doc/Documentation/spi/spidev.rst](https://www.kernel.org/doc/Documentation/spi/spidev.rst)

### 2.2.4 GPIO Controller (gpiochip)


**Interface:**

All FBOSS user space services access GPIOs via the character device interface at
`/dev/gpiochip#`. Full description can be found here.
[https://docs.kernel.org/userspace-api/gpio/chardev.html](https://docs.kernel.org/userspace-api/gpio/chardev.html)

A GPIO Line is identified by `<gpiochip, offset>` pair: `gpiochip` is a symlink
located under `/run/devmap/gpiochips/`, and offset is a non-negative offset
within the corresponding `gpiochip`.

### 2.2.5 HWMON and PMBUS Device

Typical hardware monitoring devices in FBOSS include temperature, voltage,
current and power sensors, PWM and Fan, etc. These device drivers are
developed under the Linux hardware monitoring framework.

**Interface:**

FBOSS services access hwmon devices via `/sys/class/hwmon/hwmon#`.

hwmon drivers must report values to user space with proper units, and below link
defines all the details:

* [kernel.org/doc/Documentation/hwmon/sysfs-interface](https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface)


### 2.2.6 Fan Controller (fan_ctrl)

**Interface:**

Fan control devices shall be implemented as hwmon devices and as such will follow the hwmon sysfs interface generally.

**Behavior:**

In cases where a fan is not present, all writes and reads to/from that fan must fail. For example, reading the RPM from a fan
which is not present should not return 0 or some other "bad" value, but rather the operation must return an error code.


### 2.2.7 Fan Watchdog (watchdog_fan)

**Interface:**

FBOSS adopts the standard Linux watchdog daemon to feed watchdog via character
device interface `/dev/watchdog#`. The watchdog API is defined here.
[kernel.org/doc/html/v6.4/watchdog/watchdog-api](https://www.kernel.org/doc/html/v5.9/watchdog/watchdog-api.html)

FBOSS Watchdog devices support `start`, `stop`, `ping` and `set_timeout`
operations.


### 2.2.8 Transceiver Controller (xcvr_ctrl)

**Interface:**

The `xcvr_ctrl` driver exports following sysfs entries for each transceiver port
(port_num is 1-based integer):

* `xcvr_reset_<portnum>`
  * **TYPE**: Bit
  * **Description**: Setting to 1 puts the optics in reset state, 0 takes it out
    of reset.
  * **Read/Write**: RW
* `xcvr_low_power_<portnum>`
  * **Type**: Bit
  * **Description**: 1 sets the xcvr to low power mode.
  * **Read/Write**: RW
* `xcvr_present_<portnum>`
  * **Type**: Bit
  * **Description**: 1 indicates xcvr is present, 0 indicates not present.
  * **Read/Write**: RO


### 2.2.9 LED

LEDs are accessed via their original sysfs paths since they are non-dynamic. No
symlinks are created by PlatformManager. All leds are found at `/sys/class/leds/`

**Naming:**

LEDs are named with a common scheme:

    <type><id?>_led:<color?>:status

For example:

    port10_led:blue:status
    sys_led:red:status
    fan1_led:multicolor:status
    port1_led::status

If an LED has only one color, the color will be omitted from the name. If the
LED supports the multicolor system, `multicolor` will be used as the color name.
If an LED has no `id` in the name, it is a system-level LED, for example
front-panel LEDs.

Led Types:

* `port`
* `fan`
* `psu`
* `sys`

**Interface:**

* Single-color LEDs:
  * FBOSS services control LEDs by writing `0` or `max_brightness` to
    `/sys/class/leds/<LED_NAME>/brightness` files.
* Multicolor LEDs:
  * the `multi_intensity` file in the LED directory is used to set RGB values
    (in that order)
  * Example:
    * `echo 43 226 138 > /sys/class/leds/port1:multicolor:status/multi_intensity`

A full description of the Multicolor LED API can be found here.
[docs.kernel.org/leds/leds-class-multicolor](https://docs.kernel.org/leds/leds-class-multicolor.html)
