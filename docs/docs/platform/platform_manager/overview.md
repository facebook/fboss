---
id: overview
title: PlatformManager Overview
sidebar_label: Overview
sidebar_position: 1
slug: /platform/platform_manager
---

# PlatformManager

## Introduction

The objective of PlatformManager is to generalize low level device setup and
management across platforms. This avoids platform specific customized software
logic. All the Platform specific information should be encoded in configuration
files, and the same generic software should run on all platforms going forward.

## Functions of Interest

- **Inventory Management**: A switch system could have different FRUs.
  PlatformManager is responsible for building the inventory of the FRUs in a
  switch. PlatformManager needs to monitor in real time what FRU has been
  removed or inserted into the switch. It provides an interface to software
  components within and outside of PlatformManager about the FRUs and the FRU
  EEPROM contents.
- **Dynamic Device Management**: When an FRU has been detected by the Inventory
  Management, Dynamic Device Management will load the necessary kmods and enable
  communication with the FRU. PlatformManager will also ensure that the FRU
  devices get consistent naming within each platform irrespective of how it is
  plugged in and across reboots.

## Components

- **Platform Specific Configuration (provided by Vendors):** It has information
  about the FRUs, I2C devices and how they can be accessed. The configuration
  format is defined in [platform_manager_config.thrift](https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/platform_manager_config.thrift). See here for a [sample](https://github.com/facebook/fboss/blob/main/fboss/platform/configs/sample/platform_manager.json) config.
- **Platform Agnostic Software (developed by Meta):** Meta will write and
  maintain the software. The [source code is here](https://github.com/facebook/fboss/tree/main/fboss/platform/platform_manager).

## Documentation

| Topic                                                                  | Description                                      |
| ---------------------------------------------------------------------- | ------------------------------------------------ |
| [Modeling Requirements](/docs/platform/platform_manager/modeling)      | PMUnit, IDPROM, and hardware modeling rules      |
TBD: add more documentation

## Workflow

### On Initialization

- Determine Platform Type using dmidecode set by BIOS. Details are in BMC Lite
  Specifications.
- Load the platform specific PlatformManager Config
- Discover the devices (by polling present registers or reading sysfs paths)
  based on the config and load the necessary kmods.

### Periodically

- Detect devices by listening to present polling/interrupts (by polling for now,
  since interrupts are not full-fledged)
- On device detection, use the PlatformManager Config to set up the device and
  load the necessary kmods.

## Usage

- PlatformManager will be used at provisioning/boot-up and be executed before
  any FRU/EEPROM/FW access.
- PlatformManager will be run in steady state of the switch to facilitate
  dynamic detection of devices.

## Vendor Expectations

- **Configuration**: Vendor will provide the platform specific configuration
  according to the [specification](https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/platform_manager_config.thrift)
  in fboss repository.
- **Handoff**: Vendor will run PlatformManager as part of the handoff. The running
  and testing instructions will be provided in fboss repository.
- **PCI Device IDs**: All PCI devices (e.g., FPGA) should have unique Vendor ID and
  Device ID in the PCI configuration space.
- **Dmidecode Setup**: The platform type should be obtainable through dmidecode, per
  BMC-lite specification document
- **BSP**: The BSP should follow the [BSP requirements specification](/docs/platform/bsp_development_requirements)
  to enable device discovery and setup. We expect one set of kmods per vendor.
