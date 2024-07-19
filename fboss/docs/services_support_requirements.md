# FBOSS Platform Services Support Requirements


## Revision 0.5.0

This document describes the configurations and software packages that Vendors
must provide to support FBOSS X86 Software development. FBOSS OpenBMC
requirements are described in a separate document “Meta/OpenBMC Software
Requirement”.


# FBOSS Platform Services Overview


## fan_service

`fan_service` reads temperature sensors and adjusts fan speed (or shutdown
hardware components) to achieve desired thermal requirements.


## sensor_service

`sensor_service` reads various hardware monitoring sensors regularly, and
answers clients’ requests via thrift interface. `fan_service` is one of the
clients, which requests temperature sensor values from `sensor_service`.


## data_corral_service

`data_corral_service` serves as whole chassis information aggregator, and front
panel LED controller based on chassis/FRU health


## platform_manager

`platform_manager` handles creation/deletion/initialization of platform devices.
For example, it creates and initializes I2C devices when a PIM card is plugged
and deletes these I2C devices when the card is removed.


## weutil

`weutil` is a CLI/API software package that provides chassis/FRU information
based on Meta format. It should contain the APIs which can be used by other
platform services.


## rackmon

`rackmon` service provides communication channels between Rack-level PSUs/BBUs
and rack-power management services, and it enables rack-power management
services to monitor power loss events and read/write PSU/BBU registers via
RS485. `rackmon` is only deployed in TOR switches.


## pemd/pem-util

`pem_service` reads various PEM sensors (voltage, current, etc) and shutdown
hardware components when sensor values go beyond predefined thresholds. `pemd`
and `pem-util` won’t be deployed on switches with AC PSUs.


# Requirements:

* We avoid “platform” specific source code. For each new platform, only
  configuration files are needed. Vendor needs to compile the service and run it
  with the desired configuration (specified below). Vendor should verify the
  service is able to achieve its desired objective. For example,
  `platform_manager` should be able to setup all the devices mentioned in its
  config successfully, sensor_service should be able to read all the sensors
  mentioned in its config, weutil should be able to display the EEPROM
  information of the specified FRU etc.
* Vendor needs to ensure all the tests associated with these services pass. The
  tests are located in the same folder as the service code.

<table>
   <tr>
      <th>Platform Service</th>
      <th>Configurations/Requirements</th>
   </tr>
   <tr>
      <td>fan_service</td>
      <td>
        <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/fan_service/if/fan_service_config.thrift">fan_service_config.thrift</a>
    </td>
   </tr>
   <tr>
      <td>sensor_service</td>
      <td>
        <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/sensor_service/if/sensor_config.thrift">sensor_config.thrift</a>
    </td>
   </tr>
   <tr>
      <td>platform_manager</td>
      <td>
        <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/platform_manager_config.thrift">platform_manager_config.thrift</a>
      </td>
   </tr>
   <tr>
      <td>weutil</td>
      <td>
        <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/weutil/if/weutil_config.thrift">weutil_config.thrift</a>
    </td>
   </tr>
   <tr>
      <td>fw_util</td>
      <td>
         <ul>
            <li><a href="https://github.com/facebook/fboss/blob/main/fboss/platform/fw_util/if/fw_util_config.thrift">fw_util_config.thrift</a></li>
            <li>For each command/binary used, they must be statically compiled,
            no other libraries/dependencies</li>
            <li>Must use only SPI driver, I2C driver or JTAG driver</li>
            <li>Must not use vendor specific tools</li>
         </ul>
      </td>
   </tr>
   <tr>
      <td>PEM Service</td>
      <td>
        TBD
      </td>
   </tr>
   <tr>
      <td>rackmon</td>
      <td>
         <ul>
            <li>
            Three UART devices (usually /dev/ttyUSB*) for modbus communications.
            Example: <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/rackmon/configs/interface/rackmon.conf">rackmon.conf</a></li>
            <li>
            6 GPIO Lines for Power Loss Siren. Example:
            <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/rackmon/configs/interface/rackmon_pls.conf">rackmon_pls.conf</a></li>
         </ul>
      </td>
   </tr>
   <tr>
      <td>data_corral_service</td>
      <td>
         <ul>
            <li>
            <a href="https://www.internalfb.com/code/fbsource/fbcode/fboss/platform/data_corral_service/if/data_corral_service.thrift">data_corral_service.thrift</a></li>
            <li>
            Vendor should implement sample LED logic code (APIs) based on
            Chassis/FRU health, presence, reference:
            <a href="https://github.com/facebook/fboss/blob/main/fboss/platform/data_corral_service/darwin/DarwinChassisManager.cpp">DarwinChassisManager.cpp</a></li>
         </ul>
      </td>
   </tr>
</table>



# Device Naming

It is a bad idea to reference devices using absolute sysfs/chardev paths in user
space applications, because bus/device numbers are usually assigned dynamically
and they could be changed across reboots. For example, `/dev/i2c-#` and
`/sys/class/hwmon/hwmon#` are dynamically assigned, depending on the order of
device enumeration.

In FBOSS, platform services reference devices via symbolic links, and the
symlinks ensure correct name-to-device mapping. The symbolic links are organized
in 3 levels:

* **root directory:**

    All the symlinks are located under the root directory `/run/devmap/`.

* **group directories:**

    The symlinks are categorized by groups. The supported groups are:
    `i2c-busses`, `gpiochips`, `mdio-busses`, `watchdogs`, `sensors`, `eeproms`,
    `spi-devices`, `fpgas` and `cplds`.

* **leaf symlinks:**

    The leaf symbolic links point to the corresponding device file. Leaf symlink
    files must be assigned meaningful names, containing upper-case letters,
    numbers (0-9) and underscores.

Below is the detailed file structure under `/run/devmap`. Please note: only
create symlinks for devices that are referenced by FBOSS services.

* **i2c-busses:**

    This directory contains symlinks pointing to `/dev/i2c-###` character
    device. For example:
```
/run/devmap/i2c-busses/OPTICS_1   —> /dev/i2c-5   /* I2C Adapater for PIM1 QSFP Port 1 */
…
/run/devmap/i2c-busses/OPTICS_17   —> /dev/i2c-31  /* I2C Adapter for PIM2 QSFP Port 1 */
```

Example udev rules:

`SUBSYSTEM=="i2c", ATTR{name}=="PIM1.OPTICS1:*", RUN+="/bin/ln -sf /dev/i2c-%n
/run/devmap/i2c-busses/OPTICS_1"`



* **gpiochips**

    This directory contains symlinks pointing to `/dev/gpiochip###` character
    device. For example:
```
/run/devmap/gpiochips/RACKMON_PLS  —> /dev/gpiochip1
/run/devmap/gpiochips/SCM_GPIO → /dev/gpiochip2
/run/devmap/gpiochips/PIM1_GPIO → /dev/gpiochip3
…
/run/devmap/gpiochips/PIM8_GPIO → /dev/gpiochip10
```

Example udev rules:

`SUBSYSTEM=="gpio", DEVPATH=="/devices/pci0000:00/0000:00:01.1/*/gpio-pim4/*",
RUN+="/bin/ln -sf /dev/gpiochip%n /run/devmap/gpiochips/PIM4_GPIO"`

* **mdio-busses**

    This directory contains symlinks pointing to `/dev/mdio/mdio-###` character
    devices, and each symlink is mapped to a MDIO bus.


    For example:
```
/run/devmap/mdio-busses/PIM1_MDIO1  —> /dev/mdio/mdio##
/run/devmap/mdio-busses/PIM1_MDIO2  —> /dev/mdio/mdio##
…
/run/devmap/mdio-busses/PIM8_MDIO4  —> /dev/mdio/mdio##
```

Example udev rules:

`SUBSYSTEM=="mdio_bus", DEVPATH=="/devices/pci0000:00/0000:00:01.1/*/mdio-pim1.0/*",
RUN+="/bin/ln -s /dev/mdio/mdio-pim1:0 /run/devmap/mdio-busses/PIM1_MDIO1"`



* **watchdogs**

    The directory contains symlink pointing to `/dev/watchdog#` character device.

    Usually we only enable 1 watchdog in X86 and the watchdog would be kicked by
    opensource `watchdogd` service. The watchdog device would be named
    `FBOSS_WDT`.

* `/run/devmap/watchdogs/FBOSS_WDT → /dev/watchdog0`

    Example udev rules:

    `SUBSYSTEM=="watchdog", ACTION=="add", DEVPATH=="/devices/pci0000:00/*/0000:07:00.0/watchdog*",
    RUN+="/bin/ln -sf /dev/watchdog%n /run/devmap/watchdogs/FBOSS_WDT"`


* **sensors**

    The directory contains symlinks to sensor chips’ sysfs directories.

    For hwmon chips, the symlink must be pointing to `/sys/class/hwmon/hwmon##`,
    for example,

* `/run/devmap/sensors/CPU_CORE_TEMP → /sys/class/hwmon/hwmon0`

    For non-hwmon sensor chips, such as ADC, the symlink points to the
    corresponding sysfs path. For example:

* `/run/devmap/sensors/PEM_ADC → /sys/bus/i2c/devices/12-004a/iio:device0`

    Example udev rules:

    `SUBSYSTEM=="hwmon", ATTR{name}=="coretemp", RUN+="/bin/ln -sf
    %S/class/hwmon/hwmon%n /run/devmap/sensors/CPU_CORE_TEMP"`

    `SUBSYSTEM=="iio", ATTR{name}=="max11645", RUN+="/bin/ln -sf %S%p
    /run/devmap/sensors/PEM_ADC_MAX11645"`


* **eeproms**

    The directory contains symlinks to the EEPROM files. For example,

    ```
    /run/devmap/eeproms/CHASSIS_EEPROM → /sys/bus/i2c/devices/6-0051/eeprom
    /run/devmap/eeproms/OOB_SWITCH_EEPROM → /sys/bus/spi/devices/spi2.2/eeprom
    ```

    Example udev rules:

    To be added..


* **spidevs**

    The directory contains symlinks pointing to `/dev/spidev#.#` character
    device. For example,

* `/run/devmap/spidevs/PIM1_FLASH  —> /dev/spidev1.0`

    Example udev rules:

    To be added..


* **fpgas**

    The directory contains symlinks pointing to the sysfs directory of FPGA
    devices. For example,

    `/run/devmap/fpgas/IOB_FPGA  → /sys/bus/pci/devices/0000.03.00.0/`

    Example udev rules:

    `SUBSYSTEM=="pci", KERNEL=="0000:03:00.0", RUN+="/bin/ln -sf %S%p
    /run/devmap/fpgas/IOB_FPGA"`


* **cplds**

    The directory contains symlinks pointing to the sysfs directory of CPLD
    devices. For example,

    ```
    /run/devmap/cplds/CPU_CPLD  → /sys/bus/pci/devices/0000:08:00.0/
    /run/devmap/cplds/POWER_CPLD  → /sys/bus/i2c/devices/8-0033/
    ```

    Example udev rules:

    `SUBSYSTEM=="pci", KERNEL=="0000:08:00.0", RUN+="/bin/ln -sf %S%p
    /run/devmap/cplds/CPU_CPLD"`

    `SUBSYSTEM=="i2c", ATTR{name}=="power_cpld", RUN+="/bin/ln -sf %S%p
    /run/devmap/cplds/POWER_CPLD"`
