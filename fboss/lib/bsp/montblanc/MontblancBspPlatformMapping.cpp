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
              "tcvrLaneToLedId": {
                "1": 1,
                "2": 1,
                "3": 1,
                "4": 1,
                "5": 2,
                "6": 2,
                "7": 2,
                "8": 2
              }
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
              "tcvrLaneToLedId": {
                "1": 3,
                "2": 3,
                "3": 3,
                "4": 3,
                "5": 4,
                "6": 4,
                "7": 4,
                "8": 4
              }
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
              "tcvrLaneToLedId": {
                "1": 5,
                "2": 5,
                "3": 5,
                "4": 5,
                "5": 6,
                "6": 6,
                "7": 6,
                "8": 6
              }
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
              "tcvrLaneToLedId": {
                "1": 7,
                "2": 7,
                "3": 7,
                "4": 7,
                "5": 8,
                "6": 8,
                "7": 8,
                "8": 8
              }
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
              "tcvrLaneToLedId": {
                "1": 9,
                "2": 9,
                "3": 9,
                "4": 9,
                "5": 10,
                "6": 10,
                "7": 10,
                "8": 10
              }
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
              "tcvrLaneToLedId": {
                "1": 11,
                "2": 11,
                "3": 11,
                "4": 11,
                "5": 12,
                "6": 12,
                "7": 12,
                "8": 12
              }
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
              "tcvrLaneToLedId": {
                "1": 13,
                "2": 13,
                "3": 13,
                "4": 13,
                "5": 14,
                "6": 14,
                "7": 14,
                "8": 14
              }
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
              "tcvrLaneToLedId": {
                "1": 15,
                "2": 15,
                "3": 15,
                "4": 15,
                "5": 16,
                "6": 16,
                "7": 16,
                "8": 16
              }
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
              "tcvrLaneToLedId": {
                "1": 17,
                "2": 17,
                "3": 17,
                "4": 17,
                "5": 18,
                "6": 18,
                "7": 18,
                "8": 18
              }
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
              "tcvrLaneToLedId": {
                "1": 19,
                "2": 19,
                "3": 19,
                "4": 19,
                "5": 20,
                "6": 20,
                "7": 20,
                "8": 20
              }
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
              "tcvrLaneToLedId": {
                "1": 21,
                "2": 21,
                "3": 21,
                "4": 21,
                "5": 22,
                "6": 22,
                "7": 22,
                "8": 22
              }
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
              "tcvrLaneToLedId": {
                "1": 23,
                "2": 23,
                "3": 23,
                "4": 23,
                "5": 24,
                "6": 24,
                "7": 24,
                "8": 24
              }
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
              "tcvrLaneToLedId": {
                "1": 25,
                "2": 25,
                "3": 25,
                "4": 25,
                "5": 26,
                "6": 26,
                "7": 26,
                "8": 26
              }
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
              "tcvrLaneToLedId": {
                "1": 27,
                "2": 27,
                "3": 27,
                "4": 27,
                "5": 28,
                "6": 28,
                "7": 28,
                "8": 28
              }
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
              "tcvrLaneToLedId": {
                "1": 29,
                "2": 29,
                "3": 29,
                "4": 29,
                "5": 30,
                "6": 30,
                "7": 30,
                "8": 30
              }
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
              "tcvrLaneToLedId": {
                "1": 31,
                "2": 31,
                "3": 31,
                "4": 31,
                "5": 32,
                "6": 32,
                "7": 32,
                "8": 32
              }
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
              "tcvrLaneToLedId": {
                "1": 33,
                "2": 33,
                "3": 33,
                "4": 33,
                "5": 34,
                "6": 34,
                "7": 34,
                "8": 34
              }
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
              "tcvrLaneToLedId": {
                "1": 35,
                "2": 35,
                "3": 35,
                "4": 35,
                "5": 36,
                "6": 36,
                "7": 36,
                "8": 36
              }
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
              "tcvrLaneToLedId": {
                "1": 37,
                "2": 37,
                "3": 37,
                "4": 37,
                "5": 38,
                "6": 38,
                "7": 38,
                "8": 38
              }
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
              "tcvrLaneToLedId": {
                "1": 39,
                "2": 39,
                "3": 39,
                "4": 39,
                "5": 40,
                "6": 40,
                "7": 40,
                "8": 40
              }
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
              "tcvrLaneToLedId": {
                "1": 41,
                "2": 41,
                "3": 41,
                "4": 41,
                "5": 42,
                "6": 42,
                "7": 42,
                "8": 42
              }
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
              "tcvrLaneToLedId": {
                "1": 43,
                "2": 43,
                "3": 43,
                "4": 43,
                "5": 44,
                "6": 44,
                "7": 44,
                "8": 44
              }
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
              "tcvrLaneToLedId": {
                "1": 45,
                "2": 45,
                "3": 45,
                "4": 45,
                "5": 46,
                "6": 46,
                "7": 46,
                "8": 46
              }
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
              "tcvrLaneToLedId": {
                "1": 47,
                "2": 47,
                "3": 47,
                "4": 47,
                "5": 48,
                "6": 48,
                "7": 48,
                "8": 48
              }
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
              "tcvrLaneToLedId": {
                "1": 49,
                "2": 49,
                "3": 49,
                "4": 49,
                "5": 50,
                "6": 50,
                "7": 50,
                "8": 50
              }
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
              "tcvrLaneToLedId": {
                "1": 51,
                "2": 51,
                "3": 51,
                "4": 51,
                "5": 52,
                "6": 52,
                "7": 52,
                "8": 52
              }
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
              "tcvrLaneToLedId": {
                "1": 53,
                "2": 53,
                "3": 53,
                "4": 53,
                "5": 54,
                "6": 54,
                "7": 54,
                "8": 54
              }
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
              "tcvrLaneToLedId": {
                "1": 55,
                "2": 55,
                "3": 55,
                "4": 55,
                "5": 56,
                "6": 56,
                "7": 56,
                "8": 56
              }
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
              "tcvrLaneToLedId": {
                "1": 57,
                "2": 57,
                "3": 57,
                "4": 57,
                "5": 58,
                "6": 58,
                "7": 58,
                "8": 58
              }
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
              "tcvrLaneToLedId": {
                "1": 59,
                "2": 59,
                "3": 59,
                "4": 59,
                "5": 60,
                "6": 60,
                "7": 60,
                "8": 60
              }
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
              "tcvrLaneToLedId": {
                "1": 61,
                "2": 61,
                "3": 61,
                "4": 61,
                "5": 62,
                "6": 62,
                "7": 62,
                "8": 62
              }
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
              "tcvrLaneToLedId": {
                "1": 63,
                "2": 63,
                "3": 63,
                "4": 63,
                "5": 64,
                "6": 64,
                "7": 64,
                "8": 64
              }
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
              "tcvrLaneToLedId": {
                "1": 65,
                "2": 65,
                "3": 65,
                "4": 65,
                "5": 66,
                "6": 66,
                "7": 66,
                "8": 66
              }
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
              "tcvrLaneToLedId": {
                "1": 67,
                "2": 67,
                "3": 67,
                "4": 67,
                "5": 68,
                "6": 68,
                "7": 68,
                "8": 68
              }
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
              "tcvrLaneToLedId": {
                "1": 69,
                "2": 69,
                "3": 69,
                "4": 69,
                "5": 70,
                "6": 70,
                "7": 70,
                "8": 70
              }
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
              "tcvrLaneToLedId": {
                "1": 71,
                "2": 71,
                "3": 71,
                "4": 71,
                "5": 72,
                "6": 72,
                "7": 72,
                "8": 72
              }
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
              "tcvrLaneToLedId": {
                "1": 73,
                "2": 73,
                "3": 73,
                "4": 73,
                "5": 74,
                "6": 74,
                "7": 74,
                "8": 74
              }
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
              "tcvrLaneToLedId": {
                "1": 75,
                "2": 75,
                "3": 75,
                "4": 75,
                "5": 76,
                "6": 76,
                "7": 76,
                "8": 76
              }
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
              "tcvrLaneToLedId": {
                "1": 77,
                "2": 77,
                "3": 77,
                "4": 77,
                "5": 78,
                "6": 78,
                "7": 78,
                "8": 78
              }
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
              "tcvrLaneToLedId": {
                "1": 79,
                "2": 79,
                "3": 79,
                "4": 79,
                "5": 80,
                "6": 80,
                "7": 80,
                "8": 80
              }
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
              "tcvrLaneToLedId": {
                "1": 81,
                "2": 81,
                "3": 81,
                "4": 81,
                "5": 82,
                "6": 82,
                "7": 82,
                "8": 82
              }
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
              "tcvrLaneToLedId": {
                "1": 83,
                "2": 83,
                "3": 83,
                "4": 83,
                "5": 84,
                "6": 84,
                "7": 84,
                "8": 84
              }
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
              "tcvrLaneToLedId": {
                "1": 85,
                "2": 85,
                "3": 85,
                "4": 85,
                "5": 86,
                "6": 86,
                "7": 86,
                "8": 86
              }
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
              "tcvrLaneToLedId": {
                "1": 87,
                "2": 87,
                "3": 87,
                "4": 87,
                "5": 88,
                "6": 88,
                "7": 88,
                "8": 88
              }
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
              "tcvrLaneToLedId": {
                "1": 89,
                "2": 89,
                "3": 89,
                "4": 89,
                "5": 90,
                "6": 90,
                "7": 90,
                "8": 90
              }
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
              "tcvrLaneToLedId": {
                "1": 91,
                "2": 91,
                "3": 91,
                "4": 91,
                "5": 92,
                "6": 92,
                "7": 92,
                "8": 92
              }
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
              "tcvrLaneToLedId": {
                "1": 93,
                "2": 93,
                "3": 93,
                "4": 93,
                "5": 94,
                "6": 94,
                "7": 94,
                "8": 94
              }
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
              "tcvrLaneToLedId": {
                "1": 95,
                "2": 95,
                "3": 95,
                "4": 95,
                "5": 96,
                "6": 96,
                "7": 96,
                "8": 96
              }
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
              "tcvrLaneToLedId": {
                "1": 97,
                "2": 97,
                "3": 97,
                "4": 97,
                "5": 98,
                "6": 98,
                "7": 98,
                "8": 98
              }
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
              "tcvrLaneToLedId": {
                "1": 99,
                "2": 99,
                "3": 99,
                "4": 99,
                "5": 100,
                "6": 100,
                "7": 100,
                "8": 100
              }
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
              "tcvrLaneToLedId": {
                "1": 101,
                "2": 101,
                "3": 101,
                "4": 101,
                "5": 102,
                "6": 102,
                "7": 102,
                "8": 102
              }
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
              "tcvrLaneToLedId": {
                "1": 103,
                "2": 103,
                "3": 103,
                "4": 103,
                "5": 104,
                "6": 104,
                "7": 104,
                "8": 104
              }
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
              "tcvrLaneToLedId": {
                "1": 105,
                "2": 105,
                "3": 105,
                "4": 105,
                "5": 106,
                "6": 106,
                "7": 106,
                "8": 106
              }
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
              "tcvrLaneToLedId": {
                "1": 107,
                "2": 107,
                "3": 107,
                "4": 107,
                "5": 108,
                "6": 108,
                "7": 108,
                "8": 108
              }
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
              "tcvrLaneToLedId": {
                "1": 109,
                "2": 109,
                "3": 109,
                "4": 109,
                "5": 110,
                "6": 110,
                "7": 110,
                "8": 110
              }
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
              "tcvrLaneToLedId": {
                "1": 111,
                "2": 111,
                "3": 111,
                "4": 111,
                "5": 112,
                "6": 112,
                "7": 112,
                "8": 112
              }
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
              "tcvrLaneToLedId": {
                "1": 113,
                "2": 113,
                "3": 113,
                "4": 113,
                "5": 114,
                "6": 114,
                "7": 114,
                "8": 114
              }
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
              "tcvrLaneToLedId": {
                "1": 115,
                "2": 115,
                "3": 115,
                "4": 115,
                "5": 116,
                "6": 116,
                "7": 116,
                "8": 116
              }
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
              "tcvrLaneToLedId": {
                "1": 117,
                "2": 117,
                "3": 117,
                "4": 117,
                "5": 118,
                "6": 118,
                "7": 118,
                "8": 118
              }
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
              "tcvrLaneToLedId": {
                "1": 119,
                "2": 119,
                "3": 119,
                "4": 119,
                "5": 120,
                "6": 120,
                "7": 120,
                "8": 120
              }
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
              "tcvrLaneToLedId": {
                "1": 121,
                "2": 121,
                "3": 121,
                "4": 121,
                "5": 122,
                "6": 122,
                "7": 122,
                "8": 122
              }
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
              "tcvrLaneToLedId": {
                "1": 123,
                "2": 123,
                "3": 123,
                "4": 123,
                "5": 124,
                "6": 124,
                "7": 124,
                "8": 124
              }
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
              "tcvrLaneToLedId": {
                "1": 125,
                "2": 125,
                "3": 125,
                "4": 125,
                "5": 126,
                "6": 126,
                "7": 126,
                "8": 126
              }
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
              "tcvrLaneToLedId": {
                "1": 127,
                "2": 127,
                "3": 127,
                "4": 127,
                "5": 128,
                "6": 128,
                "7": 128,
                "8": 128
              }
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
