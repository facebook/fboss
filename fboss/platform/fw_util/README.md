# Firmware Util - A FBOSS Platform Utility

## Objective
The primary functionality of this utility is to updating all firmwares in FBOSS based network switches


## Design Concept
* Configuration driven, platform sepecific firmware util configuration JSON files are located in a directory
that bear the name of the platform. We are embedding the json file in a cpp file. Our goal is to use as few Meta
specific library as possible in the TARGET files because we have specific size requirement to meet since fw_util
will be run in ramdisk and should ideally be under 1MB.

## Config Json
* Vendor needs to provide the config json file. A example of config json can be found in fw_util/darwinFwutil directory.
fw_util will read from the config json to upgrade and pull the version of several field programmable components


## Usage
Below is an example of running fw_util on darwin

Binary name (aka fw entities) will varies based on the platform and will be part of the provided json

```
[root@fboss315069332.snc1 ~]# /usr/local/bin/fw_util
usage:
fw_util <all|binary_name> <action> <binary_file>
<binary_name> : cpu_cpld, sc_scd, sc_cpld, sc_sat_cpld0, sc_sat_cpld1, fan_cpld, bios
<action> : program, verify, read, version
<binary_file> : path to binary file which is NOT supported when pulling fw version
all: only supported when pulling fw version. Ex:fw_util all version
[root@fboss315069332.snc1 ~]#
```
