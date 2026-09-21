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

Firmware Target name (aka fw entities such as bios, cpld... etc) will varies based on the platform and will be part of the provided json

```
[root@fboss315069332.snc1 ~]# /tmp/fw_util --helpon=Flags
fw_util: Warning: SetUsageMessage() never called

  Flags from fbcode/dataflow_tracing/sdk/cpp/Flags.cpp:
    -df_test_mode (At dev time, builds and displays the trace by inserting
      placeholders. Generates a link to view the built trace.) type: bool
      default: false



  Flags from fbcode/fboss/platform/fw_util/Flags.cpp:
    -config_file (Optional platform fw_util configuration file. If empty we
      pick the platform default config) type: string default: ""
    -fw_action (The firmware action (program, verify, read, version, list) that
      must be taken for a specific fpd) type: string default: ""
    -fw_binary_file (The binary file that needs to be programmed to the fpd)
      type: string default: ""
    -fw_target_name (The fpd name that needs to be programmed) type: string default: ""
[root@fboss315069332.snc1 ~]#
```
