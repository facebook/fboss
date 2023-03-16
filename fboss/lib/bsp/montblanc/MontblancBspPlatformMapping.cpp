// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;
using namespace apache::thrift;

namespace {
constexpr auto kJsonBspPlatformMappingStr = R"(
{
  "pimMapping": {
    "1": {
        "pimID": 1,
        "tcvrMapping": {
          "1": {
              "tcvrId": 1,
              "accessControl": {
                "controllerId": "1",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "1",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_1"
              },
              "ledId": [
                1,
                2
              ]
          },
          "2": {
              "tcvrId": 2,
              "accessControl": {
                "controllerId": "2",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/2/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/2/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "2",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_2"
              },
              "ledId": [
                3,
                4
              ]
          },
          "3": {
              "tcvrId": 3,
              "accessControl": {
                "controllerId": "3",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/3/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/3/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "3",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_3"
              },
              "ledId": [
                5,
                6
              ]
          },
          "4": {
              "tcvrId": 4,
              "accessControl": {
                "controllerId": "4",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/4/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/4/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "4",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_4"
              },
              "ledId": [
                7,
                8
              ]
          },
          "5": {
              "tcvrId": 5,
              "accessControl": {
                "controllerId": "5",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/5/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/5/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "5",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_5"
              },
              "ledId": [
                9,
                10
              ]
          },
          "6": {
              "tcvrId": 6,
              "accessControl": {
                "controllerId": "6",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/6/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/6/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "6",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_6"
              },
              "ledId": [
                11,
                12
              ]
          },
          "7": {
              "tcvrId": 7,
              "accessControl": {
                "controllerId": "7",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/7/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/7/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "7",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_7"
              },
              "ledId": [
                13,
                14
              ]
          },
          "8": {
              "tcvrId": 8,
              "accessControl": {
                "controllerId": "8",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/8/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/8/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "8",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_8"
              },
              "ledId": [
                15,
                16
              ]
          },
          "9": {
              "tcvrId": 9,
              "accessControl": {
                "controllerId": "9",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/9/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/9/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "9",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_9"
              },
              "ledId": [
                17,
                18
              ]
          },
          "10": {
              "tcvrId": 10,
              "accessControl": {
                "controllerId": "10",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/10/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/10/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "10",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_10"
              },
              "ledId": [
                19,
                20
              ]
          },
          "11": {
              "tcvrId": 11,
              "accessControl": {
                "controllerId": "11",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/11/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/11/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "11",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_11"
              },
              "ledId": [
                21,
                22
              ]
          },
          "12": {
              "tcvrId": 12,
              "accessControl": {
                "controllerId": "12",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/12/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/12/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "12",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_12"
              },
              "ledId": [
                23,
                24
              ]
          },
          "13": {
              "tcvrId": 13,
              "accessControl": {
                "controllerId": "13",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/13/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/13/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "13",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_13"
              },
              "ledId": [
                25,
                26
              ]
          },
          "14": {
              "tcvrId": 14,
              "accessControl": {
                "controllerId": "14",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/14/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/14/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "14",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_14"
              },
              "ledId": [
                27,
                28
              ]
          },
          "15": {
              "tcvrId": 15,
              "accessControl": {
                "controllerId": "15",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/15/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/15/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "15",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_15"
              },
              "ledId": [
                29,
                30
              ]
          },
          "16": {
              "tcvrId": 16,
              "accessControl": {
                "controllerId": "16",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/16/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/16/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "16",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_16"
              },
              "ledId": [
                31,
                32
              ]
          },
          "17": {
              "tcvrId": 17,
              "accessControl": {
                "controllerId": "17",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/17/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/17/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "17",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_17"
              },
              "ledId": [
                33,
                34
              ]
          },
          "18": {
              "tcvrId": 18,
              "accessControl": {
                "controllerId": "18",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/18/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/18/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "18",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_18"
              },
              "ledId": [
                35,
                36
              ]
          },
          "19": {
              "tcvrId": 19,
              "accessControl": {
                "controllerId": "19",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/19/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/19/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "19",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_19"
              },
              "ledId": [
                37,
                38
              ]
          },
          "20": {
              "tcvrId": 20,
              "accessControl": {
                "controllerId": "20",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/20/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/20/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "20",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_20"
              },
              "ledId": [
                39,
                40
              ]
          },
          "21": {
              "tcvrId": 21,
              "accessControl": {
                "controllerId": "21",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/21/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/21/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "21",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_21"
              },
              "ledId": [
                41,
                42
              ]
          },
          "22": {
              "tcvrId": 22,
              "accessControl": {
                "controllerId": "22",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/22/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/22/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "22",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_22"
              },
              "ledId": [
                43,
                44
              ]
          },
          "23": {
              "tcvrId": 23,
              "accessControl": {
                "controllerId": "23",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/23/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/23/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "23",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_23"
              },
              "ledId": [
                45,
                46
              ]
          },
          "24": {
              "tcvrId": 24,
              "accessControl": {
                "controllerId": "24",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/24/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/24/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "24",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_24"
              },
              "ledId": [
                47,
                48
              ]
          },
          "25": {
              "tcvrId": 25,
              "accessControl": {
                "controllerId": "25",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/25/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/25/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "25",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_25"
              },
              "ledId": [
                49,
                50
              ]
          },
          "26": {
              "tcvrId": 26,
              "accessControl": {
                "controllerId": "26",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/26/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/26/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "26",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_26"
              },
              "ledId": [
                51,
                52
              ]
          },
          "27": {
              "tcvrId": 27,
              "accessControl": {
                "controllerId": "27",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/27/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/27/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "27",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_27"
              },
              "ledId": [
                53,
                54
              ]
          },
          "28": {
              "tcvrId": 28,
              "accessControl": {
                "controllerId": "28",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/28/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/28/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "28",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_28"
              },
              "ledId": [
                55,
                56
              ]
          },
          "29": {
              "tcvrId": 29,
              "accessControl": {
                "controllerId": "29",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/29/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/29/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "29",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_29"
              },
              "ledId": [
                57,
                58
              ]
          },
          "30": {
              "tcvrId": 30,
              "accessControl": {
                "controllerId": "30",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "30",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_30"
              },
              "ledId": [
                59,
                60
              ]
          },
          "31": {
              "tcvrId": 31,
              "accessControl": {
                "controllerId": "31",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/31/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/31/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "31",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_31"
              },
              "ledId": [
                61,
                62
              ]
          },
          "32": {
              "tcvrId": 32,
              "accessControl": {
                "controllerId": "32",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/32/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/32/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "32",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_32"
              },
              "ledId": [
                63,
                64
              ]
          },
          "33": {
              "tcvrId": 33,
              "accessControl": {
                "controllerId": "33",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/33/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/33/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "33",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_33"
              },
              "ledId": [
                65,
                66
              ]
          },
          "34": {
              "tcvrId": 34,
              "accessControl": {
                "controllerId": "34",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/34/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/34/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "34",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_34"
              },
              "ledId": [
                67,
                68
              ]
          },
          "35": {
              "tcvrId": 35,
              "accessControl": {
                "controllerId": "35",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/35/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/35/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "35",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_35"
              },
              "ledId": [
                69,
                70
              ]
          },
          "36": {
              "tcvrId": 36,
              "accessControl": {
                "controllerId": "36",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/36/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/36/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "36",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_36"
              },
              "ledId": [
                71,
                72
              ]
          },
          "37": {
              "tcvrId": 37,
              "accessControl": {
                "controllerId": "37",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/37/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/37/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "37",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_37"
              },
              "ledId": [
                73,
                74
              ]
          },
          "38": {
              "tcvrId": 38,
              "accessControl": {
                "controllerId": "38",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/38/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/38/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "38",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_38"
              },
              "ledId": [
                75,
                76
              ]
          },
          "39": {
              "tcvrId": 39,
              "accessControl": {
                "controllerId": "39",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/39/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/39/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "39",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_39"
              },
              "ledId": [
                77,
                78
              ]
          },
          "40": {
              "tcvrId": 40,
              "accessControl": {
                "controllerId": "40",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/40/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/40/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "40",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_40"
              },
              "ledId": [
                79,
                80
              ]
          },
          "41": {
              "tcvrId": 41,
              "accessControl": {
                "controllerId": "41",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/41/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/41/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "41",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_41"
              },
              "ledId": [
                81,
                82
              ]
          },
          "42": {
              "tcvrId": 42,
              "accessControl": {
                "controllerId": "42",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/42/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/42/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "42",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_42"
              },
              "ledId": [
                83,
                84
              ]
          },
          "43": {
              "tcvrId": 43,
              "accessControl": {
                "controllerId": "43",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/43/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/43/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "43",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_43"
              },
              "ledId": [
                85,
                86
              ]
          },
          "44": {
              "tcvrId": 44,
              "accessControl": {
                "controllerId": "44",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/44/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/44/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "44",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_44"
              },
              "ledId": [
                87,
                88
              ]
          },
          "45": {
              "tcvrId": 45,
              "accessControl": {
                "controllerId": "45",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/45/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/45/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "45",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_45"
              },
              "ledId": [
                89,
                90
              ]
          },
          "46": {
              "tcvrId": 46,
              "accessControl": {
                "controllerId": "46",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/46/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/46/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "46",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_46"
              },
              "ledId": [
                91,
                92
              ]
          },
          "47": {
              "tcvrId": 47,
              "accessControl": {
                "controllerId": "47",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/47/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/47/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "47",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_47"
              },
              "ledId": [
                93,
                94
              ]
          },
          "48": {
              "tcvrId": 48,
              "accessControl": {
                "controllerId": "48",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/48/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/48/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "48",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_48"
              },
              "ledId": [
                95,
                96
              ]
          },
          "49": {
              "tcvrId": 49,
              "accessControl": {
                "controllerId": "49",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/49/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/49/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "49",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_49"
              },
              "ledId": [
                97,
                98
              ]
          },
          "50": {
              "tcvrId": 50,
              "accessControl": {
                "controllerId": "50",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/50/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/50/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "50",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_50"
              },
              "ledId": [
                99,
                100
              ]
          },
          "51": {
              "tcvrId": 51,
              "accessControl": {
                "controllerId": "51",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/51/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/51/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "51",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_51"
              },
              "ledId": [
                101,
                102
              ]
          },
          "52": {
              "tcvrId": 52,
              "accessControl": {
                "controllerId": "52",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/52/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/52/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "52",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_52"
              },
              "ledId": [
                103,
                104
              ]
          },
          "53": {
              "tcvrId": 53,
              "accessControl": {
                "controllerId": "53",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/53/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/53/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "53",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_53"
              },
              "ledId": [
                105,
                106
              ]
          },
          "54": {
              "tcvrId": 54,
              "accessControl": {
                "controllerId": "54",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/54/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/54/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "54",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_54"
              },
              "ledId": [
                107,
                108
              ]
          },
          "55": {
              "tcvrId": 55,
              "accessControl": {
                "controllerId": "55",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/55/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/55/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "55",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_55"
              },
              "ledId": [
                109,
                110
              ]
          },
          "56": {
              "tcvrId": 56,
              "accessControl": {
                "controllerId": "56",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/56/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/56/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "56",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_56"
              },
              "ledId": [
                111,
                112
              ]
          },
          "57": {
              "tcvrId": 57,
              "accessControl": {
                "controllerId": "57",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/57/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/57/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "57",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_57"
              },
              "ledId": [
                113,
                114
              ]
          },
          "58": {
              "tcvrId": 58,
              "accessControl": {
                "controllerId": "58",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/58/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/58/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "58",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_58"
              },
              "ledId": [
                115,
                116
              ]
          },
          "59": {
              "tcvrId": 59,
              "accessControl": {
                "controllerId": "59",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/59/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/59/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "59",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_59"
              },
              "ledId": [
                117,
                118
              ]
          },
          "60": {
              "tcvrId": 60,
              "accessControl": {
                "controllerId": "60",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/60/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/60/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "60",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_60"
              },
              "ledId": [
                119,
                120
              ]
          },
          "61": {
              "tcvrId": 61,
              "accessControl": {
                "controllerId": "61",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/61/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/61/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "61",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_61"
              },
              "ledId": [
                121,
                122
              ]
          },
          "62": {
              "tcvrId": 62,
              "accessControl": {
                "controllerId": "62",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/62/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/62/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "62",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_62"
              },
              "ledId": [
                123,
                124
              ]
          },
          "63": {
              "tcvrId": 63,
              "accessControl": {
                "controllerId": "63",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/63/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/63/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "63",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_63"
              },
              "ledId": [
                125,
                126
              ]
          },
          "64": {
              "tcvrId": 64,
              "accessControl": {
                "controllerId": "64",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/64/cpld_qsfpdd_intr_reset_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/64/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "64",
                "type": 1,
                "devicePath": "/run/devmap/i2c-buses/OPTICS_64"
              },
              "ledId": [
                127,
                128
              ]
          }
        },
        "phyMapping": {

        },
        "phyIOControllers": {

        },
        "ledMapping": {
          "1": {
              "id": 1,
              "bluePath": "/sys/class/leds:blue:pled1_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled1_1/brightness",
              "transceiverId": 1
          },
          "2": {
              "id": 2,
              "bluePath": "/sys/class/leds:blue:pled1_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled1_2/brightness",
              "transceiverId": 1
          },
          "3": {
              "id": 3,
              "bluePath": "/sys/class/leds:blue:pled2_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled2_1/brightness",
              "transceiverId": 2
          },
          "4": {
              "id": 4,
              "bluePath": "/sys/class/leds:blue:pled2_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled2_2/brightness",
              "transceiverId": 2
          },
          "5": {
              "id": 5,
              "bluePath": "/sys/class/leds:blue:pled3_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled3_1/brightness",
              "transceiverId": 3
          },
          "6": {
              "id": 6,
              "bluePath": "/sys/class/leds:blue:pled3_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled3_2/brightness",
              "transceiverId": 3
          },
          "7": {
              "id": 7,
              "bluePath": "/sys/class/leds:blue:pled4_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled4_1/brightness",
              "transceiverId": 4
          },
          "8": {
              "id": 8,
              "bluePath": "/sys/class/leds:blue:pled4_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled4_2/brightness",
              "transceiverId": 4
          },
          "9": {
              "id": 9,
              "bluePath": "/sys/class/leds:blue:pled5_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled5_1/brightness",
              "transceiverId": 5
          },
          "10": {
              "id": 10,
              "bluePath": "/sys/class/leds:blue:pled5_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled5_2/brightness",
              "transceiverId": 5
          },
          "11": {
              "id": 11,
              "bluePath": "/sys/class/leds:blue:pled6_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled6_1/brightness",
              "transceiverId": 6
          },
          "12": {
              "id": 12,
              "bluePath": "/sys/class/leds:blue:pled6_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled6_2/brightness",
              "transceiverId": 6
          },
          "13": {
              "id": 13,
              "bluePath": "/sys/class/leds:blue:pled7_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled7_1/brightness",
              "transceiverId": 7
          },
          "14": {
              "id": 14,
              "bluePath": "/sys/class/leds:blue:pled7_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled7_2/brightness",
              "transceiverId": 7
          },
          "15": {
              "id": 15,
              "bluePath": "/sys/class/leds:blue:pled8_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled8_1/brightness",
              "transceiverId": 8
          },
          "16": {
              "id": 16,
              "bluePath": "/sys/class/leds:blue:pled8_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled8_2/brightness",
              "transceiverId": 8
          },
          "17": {
              "id": 17,
              "bluePath": "/sys/class/leds:blue:pled9_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled9_1/brightness",
              "transceiverId": 9
          },
          "18": {
              "id": 18,
              "bluePath": "/sys/class/leds:blue:pled9_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled9_2/brightness",
              "transceiverId": 9
          },
          "19": {
              "id": 19,
              "bluePath": "/sys/class/leds:blue:pled10_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled10_1/brightness",
              "transceiverId": 10
          },
          "20": {
              "id": 20,
              "bluePath": "/sys/class/leds:blue:pled10_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled10_2/brightness",
              "transceiverId": 10
          },
          "21": {
              "id": 21,
              "bluePath": "/sys/class/leds:blue:pled11_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled11_1/brightness",
              "transceiverId": 11
          },
          "22": {
              "id": 22,
              "bluePath": "/sys/class/leds:blue:pled11_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled11_2/brightness",
              "transceiverId": 11
          },
          "23": {
              "id": 23,
              "bluePath": "/sys/class/leds:blue:pled12_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled12_1/brightness",
              "transceiverId": 12
          },
          "24": {
              "id": 24,
              "bluePath": "/sys/class/leds:blue:pled12_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled12_2/brightness",
              "transceiverId": 12
          },
          "25": {
              "id": 25,
              "bluePath": "/sys/class/leds:blue:pled13_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled13_1/brightness",
              "transceiverId": 13
          },
          "26": {
              "id": 26,
              "bluePath": "/sys/class/leds:blue:pled13_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled13_2/brightness",
              "transceiverId": 13
          },
          "27": {
              "id": 27,
              "bluePath": "/sys/class/leds:blue:pled14_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled14_1/brightness",
              "transceiverId": 14
          },
          "28": {
              "id": 28,
              "bluePath": "/sys/class/leds:blue:pled14_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled14_2/brightness",
              "transceiverId": 14
          },
          "29": {
              "id": 29,
              "bluePath": "/sys/class/leds:blue:pled15_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled15_1/brightness",
              "transceiverId": 15
          },
          "30": {
              "id": 30,
              "bluePath": "/sys/class/leds:blue:pled15_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled15_2/brightness",
              "transceiverId": 15
          },
          "31": {
              "id": 31,
              "bluePath": "/sys/class/leds:blue:pled16_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled16_1/brightness",
              "transceiverId": 16
          },
          "32": {
              "id": 32,
              "bluePath": "/sys/class/leds:blue:pled16_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled16_2/brightness",
              "transceiverId": 16
          },
          "33": {
              "id": 33,
              "bluePath": "/sys/class/leds:blue:pled17_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled17_1/brightness",
              "transceiverId": 17
          },
          "34": {
              "id": 34,
              "bluePath": "/sys/class/leds:blue:pled17_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled17_2/brightness",
              "transceiverId": 17
          },
          "35": {
              "id": 35,
              "bluePath": "/sys/class/leds:blue:pled18_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled18_1/brightness",
              "transceiverId": 18
          },
          "36": {
              "id": 36,
              "bluePath": "/sys/class/leds:blue:pled18_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled18_2/brightness",
              "transceiverId": 18
          },
          "37": {
              "id": 37,
              "bluePath": "/sys/class/leds:blue:pled19_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled19_1/brightness",
              "transceiverId": 19
          },
          "38": {
              "id": 38,
              "bluePath": "/sys/class/leds:blue:pled19_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled19_2/brightness",
              "transceiverId": 19
          },
          "39": {
              "id": 39,
              "bluePath": "/sys/class/leds:blue:pled20_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled20_1/brightness",
              "transceiverId": 20
          },
          "40": {
              "id": 40,
              "bluePath": "/sys/class/leds:blue:pled20_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled20_2/brightness",
              "transceiverId": 20
          },
          "41": {
              "id": 41,
              "bluePath": "/sys/class/leds:blue:pled21_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled21_1/brightness",
              "transceiverId": 21
          },
          "42": {
              "id": 42,
              "bluePath": "/sys/class/leds:blue:pled21_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled21_2/brightness",
              "transceiverId": 21
          },
          "43": {
              "id": 43,
              "bluePath": "/sys/class/leds:blue:pled22_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled22_1/brightness",
              "transceiverId": 22
          },
          "44": {
              "id": 44,
              "bluePath": "/sys/class/leds:blue:pled22_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled22_2/brightness",
              "transceiverId": 22
          },
          "45": {
              "id": 45,
              "bluePath": "/sys/class/leds:blue:pled23_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled23_1/brightness",
              "transceiverId": 23
          },
          "46": {
              "id": 46,
              "bluePath": "/sys/class/leds:blue:pled23_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled23_2/brightness",
              "transceiverId": 23
          },
          "47": {
              "id": 47,
              "bluePath": "/sys/class/leds:blue:pled24_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled24_1/brightness",
              "transceiverId": 24
          },
          "48": {
              "id": 48,
              "bluePath": "/sys/class/leds:blue:pled24_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled24_2/brightness",
              "transceiverId": 24
          },
          "49": {
              "id": 49,
              "bluePath": "/sys/class/leds:blue:pled25_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled25_1/brightness",
              "transceiverId": 25
          },
          "50": {
              "id": 50,
              "bluePath": "/sys/class/leds:blue:pled25_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled25_2/brightness",
              "transceiverId": 25
          },
          "51": {
              "id": 51,
              "bluePath": "/sys/class/leds:blue:pled26_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled26_1/brightness",
              "transceiverId": 26
          },
          "52": {
              "id": 52,
              "bluePath": "/sys/class/leds:blue:pled26_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled26_2/brightness",
              "transceiverId": 26
          },
          "53": {
              "id": 53,
              "bluePath": "/sys/class/leds:blue:pled27_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled27_1/brightness",
              "transceiverId": 27
          },
          "54": {
              "id": 54,
              "bluePath": "/sys/class/leds:blue:pled27_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled27_2/brightness",
              "transceiverId": 27
          },
          "55": {
              "id": 55,
              "bluePath": "/sys/class/leds:blue:pled28_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled28_1/brightness",
              "transceiverId": 28
          },
          "56": {
              "id": 56,
              "bluePath": "/sys/class/leds:blue:pled28_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled28_2/brightness",
              "transceiverId": 28
          },
          "57": {
              "id": 57,
              "bluePath": "/sys/class/leds:blue:pled29_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled29_1/brightness",
              "transceiverId": 29
          },
          "58": {
              "id": 58,
              "bluePath": "/sys/class/leds:blue:pled29_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled29_2/brightness",
              "transceiverId": 29
          },
          "59": {
              "id": 59,
              "bluePath": "/sys/class/leds:blue:pled30_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled30_1/brightness",
              "transceiverId": 30
          },
          "60": {
              "id": 60,
              "bluePath": "/sys/class/leds:blue:pled30_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled30_2/brightness",
              "transceiverId": 30
          },
          "61": {
              "id": 61,
              "bluePath": "/sys/class/leds:blue:pled31_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled31_1/brightness",
              "transceiverId": 31
          },
          "62": {
              "id": 62,
              "bluePath": "/sys/class/leds:blue:pled31_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled31_2/brightness",
              "transceiverId": 31
          },
          "63": {
              "id": 63,
              "bluePath": "/sys/class/leds:blue:pled32_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled32_1/brightness",
              "transceiverId": 32
          },
          "64": {
              "id": 64,
              "bluePath": "/sys/class/leds:blue:pled32_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled32_2/brightness",
              "transceiverId": 32
          },
          "65": {
              "id": 65,
              "bluePath": "/sys/class/leds:blue:pled33_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled33_1/brightness",
              "transceiverId": 33
          },
          "66": {
              "id": 66,
              "bluePath": "/sys/class/leds:blue:pled33_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled33_2/brightness",
              "transceiverId": 33
          },
          "67": {
              "id": 67,
              "bluePath": "/sys/class/leds:blue:pled34_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled34_1/brightness",
              "transceiverId": 34
          },
          "68": {
              "id": 68,
              "bluePath": "/sys/class/leds:blue:pled34_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled34_2/brightness",
              "transceiverId": 34
          },
          "69": {
              "id": 69,
              "bluePath": "/sys/class/leds:blue:pled35_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled35_1/brightness",
              "transceiverId": 35
          },
          "70": {
              "id": 70,
              "bluePath": "/sys/class/leds:blue:pled35_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled35_2/brightness",
              "transceiverId": 35
          },
          "71": {
              "id": 71,
              "bluePath": "/sys/class/leds:blue:pled36_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled36_1/brightness",
              "transceiverId": 36
          },
          "72": {
              "id": 72,
              "bluePath": "/sys/class/leds:blue:pled36_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled36_2/brightness",
              "transceiverId": 36
          },
          "73": {
              "id": 73,
              "bluePath": "/sys/class/leds:blue:pled37_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled37_1/brightness",
              "transceiverId": 37
          },
          "74": {
              "id": 74,
              "bluePath": "/sys/class/leds:blue:pled37_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled37_2/brightness",
              "transceiverId": 37
          },
          "75": {
              "id": 75,
              "bluePath": "/sys/class/leds:blue:pled38_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled38_1/brightness",
              "transceiverId": 38
          },
          "76": {
              "id": 76,
              "bluePath": "/sys/class/leds:blue:pled38_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled38_2/brightness",
              "transceiverId": 38
          },
          "77": {
              "id": 77,
              "bluePath": "/sys/class/leds:blue:pled39_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled39_1/brightness",
              "transceiverId": 39
          },
          "78": {
              "id": 78,
              "bluePath": "/sys/class/leds:blue:pled39_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled39_2/brightness",
              "transceiverId": 39
          },
          "79": {
              "id": 79,
              "bluePath": "/sys/class/leds:blue:pled40_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled40_1/brightness",
              "transceiverId": 40
          },
          "80": {
              "id": 80,
              "bluePath": "/sys/class/leds:blue:pled40_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled40_2/brightness",
              "transceiverId": 40
          },
          "81": {
              "id": 81,
              "bluePath": "/sys/class/leds:blue:pled41_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled41_1/brightness",
              "transceiverId": 41
          },
          "82": {
              "id": 82,
              "bluePath": "/sys/class/leds:blue:pled41_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled41_2/brightness",
              "transceiverId": 41
          },
          "83": {
              "id": 83,
              "bluePath": "/sys/class/leds:blue:pled42_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled42_1/brightness",
              "transceiverId": 42
          },
          "84": {
              "id": 84,
              "bluePath": "/sys/class/leds:blue:pled42_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled42_2/brightness",
              "transceiverId": 42
          },
          "85": {
              "id": 85,
              "bluePath": "/sys/class/leds:blue:pled43_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled43_1/brightness",
              "transceiverId": 43
          },
          "86": {
              "id": 86,
              "bluePath": "/sys/class/leds:blue:pled43_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled43_2/brightness",
              "transceiverId": 43
          },
          "87": {
              "id": 87,
              "bluePath": "/sys/class/leds:blue:pled44_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled44_1/brightness",
              "transceiverId": 44
          },
          "88": {
              "id": 88,
              "bluePath": "/sys/class/leds:blue:pled44_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled44_2/brightness",
              "transceiverId": 44
          },
          "89": {
              "id": 89,
              "bluePath": "/sys/class/leds:blue:pled45_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled45_1/brightness",
              "transceiverId": 45
          },
          "90": {
              "id": 90,
              "bluePath": "/sys/class/leds:blue:pled45_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled45_2/brightness",
              "transceiverId": 45
          },
          "91": {
              "id": 91,
              "bluePath": "/sys/class/leds:blue:pled46_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled46_1/brightness",
              "transceiverId": 46
          },
          "92": {
              "id": 92,
              "bluePath": "/sys/class/leds:blue:pled46_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled46_2/brightness",
              "transceiverId": 46
          },
          "93": {
              "id": 93,
              "bluePath": "/sys/class/leds:blue:pled47_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled47_1/brightness",
              "transceiverId": 47
          },
          "94": {
              "id": 94,
              "bluePath": "/sys/class/leds:blue:pled47_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled47_2/brightness",
              "transceiverId": 47
          },
          "95": {
              "id": 95,
              "bluePath": "/sys/class/leds:blue:pled48_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled48_1/brightness",
              "transceiverId": 48
          },
          "96": {
              "id": 96,
              "bluePath": "/sys/class/leds:blue:pled48_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled48_2/brightness",
              "transceiverId": 48
          },
          "97": {
              "id": 97,
              "bluePath": "/sys/class/leds:blue:pled49_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled49_1/brightness",
              "transceiverId": 49
          },
          "98": {
              "id": 98,
              "bluePath": "/sys/class/leds:blue:pled49_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled49_2/brightness",
              "transceiverId": 49
          },
          "99": {
              "id": 99,
              "bluePath": "/sys/class/leds:blue:pled50_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled50_1/brightness",
              "transceiverId": 50
          },
          "100": {
              "id": 100,
              "bluePath": "/sys/class/leds:blue:pled50_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled50_2/brightness",
              "transceiverId": 50
          },
          "101": {
              "id": 101,
              "bluePath": "/sys/class/leds:blue:pled51_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled51_1/brightness",
              "transceiverId": 51
          },
          "102": {
              "id": 102,
              "bluePath": "/sys/class/leds:blue:pled51_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled51_2/brightness",
              "transceiverId": 51
          },
          "103": {
              "id": 103,
              "bluePath": "/sys/class/leds:blue:pled52_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled52_1/brightness",
              "transceiverId": 52
          },
          "104": {
              "id": 104,
              "bluePath": "/sys/class/leds:blue:pled52_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled52_2/brightness",
              "transceiverId": 52
          },
          "105": {
              "id": 105,
              "bluePath": "/sys/class/leds:blue:pled53_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled53_1/brightness",
              "transceiverId": 53
          },
          "106": {
              "id": 106,
              "bluePath": "/sys/class/leds:blue:pled53_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled53_2/brightness",
              "transceiverId": 53
          },
          "107": {
              "id": 107,
              "bluePath": "/sys/class/leds:blue:pled54_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled54_1/brightness",
              "transceiverId": 54
          },
          "108": {
              "id": 108,
              "bluePath": "/sys/class/leds:blue:pled54_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled54_2/brightness",
              "transceiverId": 54
          },
          "109": {
              "id": 109,
              "bluePath": "/sys/class/leds:blue:pled55_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled55_1/brightness",
              "transceiverId": 55
          },
          "110": {
              "id": 110,
              "bluePath": "/sys/class/leds:blue:pled55_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled55_2/brightness",
              "transceiverId": 55
          },
          "111": {
              "id": 111,
              "bluePath": "/sys/class/leds:blue:pled56_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled56_1/brightness",
              "transceiverId": 56
          },
          "112": {
              "id": 112,
              "bluePath": "/sys/class/leds:blue:pled56_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled56_2/brightness",
              "transceiverId": 56
          },
          "113": {
              "id": 113,
              "bluePath": "/sys/class/leds:blue:pled57_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled57_1/brightness",
              "transceiverId": 57
          },
          "114": {
              "id": 114,
              "bluePath": "/sys/class/leds:blue:pled57_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled57_2/brightness",
              "transceiverId": 57
          },
          "115": {
              "id": 115,
              "bluePath": "/sys/class/leds:blue:pled58_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled58_1/brightness",
              "transceiverId": 58
          },
          "116": {
              "id": 116,
              "bluePath": "/sys/class/leds:blue:pled58_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled58_2/brightness",
              "transceiverId": 58
          },
          "117": {
              "id": 117,
              "bluePath": "/sys/class/leds:blue:pled59_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled59_1/brightness",
              "transceiverId": 59
          },
          "118": {
              "id": 118,
              "bluePath": "/sys/class/leds:blue:pled59_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled59_2/brightness",
              "transceiverId": 59
          },
          "119": {
              "id": 119,
              "bluePath": "/sys/class/leds:blue:pled60_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled60_1/brightness",
              "transceiverId": 60
          },
          "120": {
              "id": 120,
              "bluePath": "/sys/class/leds:blue:pled60_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled60_2/brightness",
              "transceiverId": 60
          },
          "121": {
              "id": 121,
              "bluePath": "/sys/class/leds:blue:pled61_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled61_1/brightness",
              "transceiverId": 61
          },
          "122": {
              "id": 122,
              "bluePath": "/sys/class/leds:blue:pled61_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled61_2/brightness",
              "transceiverId": 61
          },
          "123": {
              "id": 123,
              "bluePath": "/sys/class/leds:blue:pled62_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled62_1/brightness",
              "transceiverId": 62
          },
          "124": {
              "id": 124,
              "bluePath": "/sys/class/leds:blue:pled62_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled62_2/brightness",
              "transceiverId": 62
          },
          "125": {
              "id": 125,
              "bluePath": "/sys/class/leds:blue:pled63_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled63_1/brightness",
              "transceiverId": 63
          },
          "126": {
              "id": 126,
              "bluePath": "/sys/class/leds:blue:pled63_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled63_2/brightness",
              "transceiverId": 63
          },
          "127": {
              "id": 127,
              "bluePath": "/sys/class/leds:blue:pled64_1/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled64_1/brightness",
              "transceiverId": 64
          },
          "128": {
              "id": 128,
              "bluePath": "/sys/class/leds:blue:pled64_2/brightness",
              "yellowPath": "/sys/class/leds:yellow:pled64_2/brightness",
              "transceiverId": 64
          }
        }
    }
  }
}
)";

static BspPlatformMappingThrift buildMontblancPlatformMapping() {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(kJsonBspPlatformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

MontblancBspPlatformMapping::MontblancBspPlatformMapping()
    : BspPlatformMapping(buildMontblancPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
