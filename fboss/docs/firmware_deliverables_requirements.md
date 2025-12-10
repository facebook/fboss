# **Firmware Repository Specification**
=====================================

### Version 1.0.0

## Scope
--------

This document outlines the requirements for the firmware repository, including directory structure, file naming conventions, and contents. The package is designed to support automated firmware ingestion, internal firmware validation, provisioning firmware readiness, and firmware deployment to the respective hardware platform.

## Directory Structure
----------------------

The firmware package for a given platform (e.g., `xylophia9000`) should be organized as follows:

```
xylophia9000/
├── firmware/
│ └── package_0/
│ ├── P_xylophia9000_F_bcm53134image-p4_V_1.2.bin
│ ├── P_xylophia9000_F_smb_fpga_X_stripped_V_4.14.abit
│ ├── P_xylophia9000_F_fan_cpld_V_1.9.astp
│ ├── P_xylophia9000_F_smb_fpga_V_4.14.abit
│ ├── P_xylophia9000_F_bios_V_lcc-64m-38965988.rom
│ ├── P_xylophia9000_F_scm_cpld_V_4.16.astp
│ ├── README.md
│ └── evt/ (optional early EVT repo if images are not compatible with DVT due to critical HW changes)
│   └── package_0/
```

## Packages
---------

* At least two packages (`package_0` and `package_1`) must exist at the root of the firmware repository.
* For the initial firmware release, the difference between `package_0` and `package_1` must be from a version readout standpoint. Otherwise, the two firmware packages are identical.
* This allows for early upgrade/downgrade testing.

## File Naming Conventions
-------------------------

### Firmware Files

Each firmware file follows the naming convention:

`P_<Platform>_F_<fw_target_name>_V_<Version>.<extension>`

  Where:

* `P_` indicates the platform or generic firmware name.
* `<Platform>` is the platform name (e.g., `xylophia9000`) which is the same as the platform name from the eeprom/dmidecode content, or a generic name which encompasses firmware that belongs to specific HW (e.g., `iceLakeD` for P_iceLakeD_F_bios_V_NL404.bin).
* `F_` indicates the firmware name, which must match the names listed in `fw_util.json`.
* `<fw_target_name>` is the name of target name found in fw_util (e.g., `smb_cpld`).
* `_V_` indicates the version.
* `<Version>` is the version number (e.g., `1.2`).
* `<extension>` is the file type (e.g., `.bin`, `.abit`, `.astp`, `.rom`).

## File Contents
--------------

### Firmware Files

These files contain the firmware images for various components of the Xylophia9000 platform.

### readme.md

This file should include:

* Release version and date
* Change log
* Any relevant warnings or cautions

### testsPlanPerformed.md
This file should list what/how the test was performed for each firmware changes. We don't need the actual log results of the tests in this repo. We simply needs to understand how it was tested and will follow up with Vendors if we have questions.

### Respin Hardware

Firmware for respin HW should be backward compatible with regular HW for the given platform.

### Optional EVT Directory

For cases where early EVT images are not compatible with DVT HW, vendors can include an `evt` directory containing EVT firmware images. There must be `packages_X` directory inside the `evt` directory. Please see directory structure in earlier in this document.

### Other Non-Provisioning Firmware

Firmware that will not be part of the provisioning process (e.g., a DPM firmware) should be part of another directory named `others`.
