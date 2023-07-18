// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
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
                "controllerId": "accessController-1",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-1",
                "type": 1,
                "devicePath": "/dev/i2c-73"
              },
              "tcvrLaneToLedId": {

              }
          },
          "2": {
              "tcvrId": 2,
              "accessControl": {
                "controllerId": "accessController-2",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-2",
                "type": 1,
                "devicePath": "/dev/i2c-74"
              },
              "tcvrLaneToLedId": {

              }
          },
          "3": {
              "tcvrId": 3,
              "accessControl": {
                "controllerId": "accessController-3",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-3",
                "type": 1,
                "devicePath": "/dev/i2c-75"
              },
              "tcvrLaneToLedId": {

              }
          },
          "4": {
              "tcvrId": 4,
              "accessControl": {
                "controllerId": "accessController-4",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-4",
                "type": 1,
                "devicePath": "/dev/i2c-76"
              },
              "tcvrLaneToLedId": {

              }
          },
          "5": {
              "tcvrId": 5,
              "accessControl": {
                "controllerId": "accessController-5",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-5",
                "type": 1,
                "devicePath": "/dev/i2c-77"
              },
              "tcvrLaneToLedId": {

              }
          },
          "6": {
              "tcvrId": 6,
              "accessControl": {
                "controllerId": "accessController-6",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-6",
                "type": 1,
                "devicePath": "/dev/i2c-78"
              },
              "tcvrLaneToLedId": {

              }
          },
          "7": {
              "tcvrId": 7,
              "accessControl": {
                "controllerId": "accessController-7",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-7",
                "type": 1,
                "devicePath": "/dev/i2c-79"
              },
              "tcvrLaneToLedId": {

              }
          },
          "8": {
              "tcvrId": 8,
              "accessControl": {
                "controllerId": "accessController-8",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-8",
                "type": 1,
                "devicePath": "/dev/i2c-80"
              },
              "tcvrLaneToLedId": {

              }
          },
          "9": {
              "tcvrId": 9,
              "accessControl": {
                "controllerId": "accessController-9",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-9",
                "type": 1,
                "devicePath": "/dev/i2c-81"
              },
              "tcvrLaneToLedId": {

              }
          },
          "10": {
              "tcvrId": 10,
              "accessControl": {
                "controllerId": "accessController-10",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-10",
                "type": 1,
                "devicePath": "/dev/i2c-82"
              },
              "tcvrLaneToLedId": {

              }
          },
          "11": {
              "tcvrId": 11,
              "accessControl": {
                "controllerId": "accessController-11",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-11",
                "type": 1,
                "devicePath": "/dev/i2c-83"
              },
              "tcvrLaneToLedId": {

              }
          },
          "12": {
              "tcvrId": 12,
              "accessControl": {
                "controllerId": "accessController-12",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-12",
                "type": 1,
                "devicePath": "/dev/i2c-84"
              },
              "tcvrLaneToLedId": {

              }
          },
          "13": {
              "tcvrId": 13,
              "accessControl": {
                "controllerId": "accessController-13",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-13",
                "type": 1,
                "devicePath": "/dev/i2c-85"
              },
              "tcvrLaneToLedId": {

              }
          },
          "14": {
              "tcvrId": 14,
              "accessControl": {
                "controllerId": "accessController-14",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-14",
                "type": 1,
                "devicePath": "/dev/i2c-86"
              },
              "tcvrLaneToLedId": {

              }
          },
          "15": {
              "tcvrId": 15,
              "accessControl": {
                "controllerId": "accessController-15",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-15",
                "type": 1,
                "devicePath": "/dev/i2c-87"
              },
              "tcvrLaneToLedId": {

              }
          },
          "16": {
              "tcvrId": 16,
              "accessControl": {
                "controllerId": "accessController-16",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-16",
                "type": 1,
                "devicePath": "/dev/i2c-88"
              },
              "tcvrLaneToLedId": {

              }
          },
          "17": {
              "tcvrId": 17,
              "accessControl": {
                "controllerId": "accessController-17",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-17",
                "type": 1,
                "devicePath": "/dev/i2c-89"
              },
              "tcvrLaneToLedId": {

              }
          },
          "18": {
              "tcvrId": 18,
              "accessControl": {
                "controllerId": "accessController-18",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_reset_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_intr_present_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-18",
                "type": 1,
                "devicePath": "/dev/i2c-90"
              },
              "tcvrLaneToLedId": {

              }
          },
          "19": {
              "tcvrId": 19,
              "accessControl": {
                "controllerId": "accessController-19",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-19",
                "type": 1,
                "devicePath": "/dev/i2c-91"
              },
              "tcvrLaneToLedId": {

              }
          },
          "20": {
              "tcvrId": 20,
              "accessControl": {
                "controllerId": "accessController-20",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-20",
                "type": 1,
                "devicePath": "/dev/i2c-92"
              },
              "tcvrLaneToLedId": {

              }
          },
          "21": {
              "tcvrId": 21,
              "accessControl": {
                "controllerId": "accessController-21",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-21",
                "type": 1,
                "devicePath": "/dev/i2c-93"
              },
              "tcvrLaneToLedId": {

              }
          },
          "22": {
              "tcvrId": 22,
              "accessControl": {
                "controllerId": "accessController-22",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-22",
                "type": 1,
                "devicePath": "/dev/i2c-94"
              },
              "tcvrLaneToLedId": {

              }
          },
          "23": {
              "tcvrId": 23,
              "accessControl": {
                "controllerId": "accessController-23",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-23",
                "type": 1,
                "devicePath": "/dev/i2c-95"
              },
              "tcvrLaneToLedId": {

              }
          },
          "24": {
              "tcvrId": 24,
              "accessControl": {
                "controllerId": "accessController-24",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-24",
                "type": 1,
                "devicePath": "/dev/i2c-96"
              },
              "tcvrLaneToLedId": {

              }
          },
          "25": {
              "tcvrId": 25,
              "accessControl": {
                "controllerId": "accessController-25",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-25",
                "type": 1,
                "devicePath": "/dev/i2c-97"
              },
              "tcvrLaneToLedId": {

              }
          },
          "26": {
              "tcvrId": 26,
              "accessControl": {
                "controllerId": "accessController-26",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-26",
                "type": 1,
                "devicePath": "/dev/i2c-98"
              },
              "tcvrLaneToLedId": {

              }
          },
          "27": {
              "tcvrId": 27,
              "accessControl": {
                "controllerId": "accessController-27",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-27",
                "type": 1,
                "devicePath": "/dev/i2c-99"
              },
              "tcvrLaneToLedId": {

              }
          },
          "28": {
              "tcvrId": 28,
              "accessControl": {
                "controllerId": "accessController-28",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-28",
                "type": 1,
                "devicePath": "/dev/i2c-100"
              },
              "tcvrLaneToLedId": {

              }
          },
          "29": {
              "tcvrId": 29,
              "accessControl": {
                "controllerId": "accessController-29",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-29",
                "type": 1,
                "devicePath": "/dev/i2c-101"
              },
              "tcvrLaneToLedId": {

              }
          },
          "30": {
              "tcvrId": 30,
              "accessControl": {
                "controllerId": "accessController-30",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-30",
                "type": 1,
                "devicePath": "/dev/i2c-102"
              },
              "tcvrLaneToLedId": {

              }
          },
          "31": {
              "tcvrId": 31,
              "accessControl": {
                "controllerId": "accessController-31",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-31",
                "type": 1,
                "devicePath": "/dev/i2c-103"
              },
              "tcvrLaneToLedId": {

              }
          },
          "32": {
              "tcvrId": 32,
              "accessControl": {
                "controllerId": "accessController-32",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-32",
                "type": 1,
                "devicePath": "/dev/i2c-104"
              },
              "tcvrLaneToLedId": {

              }
          },
          "33": {
              "tcvrId": 33,
              "accessControl": {
                "controllerId": "accessController-33",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-33",
                "type": 1,
                "devicePath": "/dev/i2c-105"
              },
              "tcvrLaneToLedId": {

              }
          },
          "34": {
              "tcvrId": 34,
              "accessControl": {
                "controllerId": "accessController-34",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-34",
                "type": 1,
                "devicePath": "/dev/i2c-106"
              },
              "tcvrLaneToLedId": {

              }
          },
          "35": {
              "tcvrId": 35,
              "accessControl": {
                "controllerId": "accessController-35",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-35",
                "type": 1,
                "devicePath": "/dev/i2c-107"
              },
              "tcvrLaneToLedId": {

              }
          },
          "36": {
              "tcvrId": 36,
              "accessControl": {
                "controllerId": "accessController-36",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_reset_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_intr_present_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-36",
                "type": 1,
                "devicePath": "/dev/i2c-108"
              },
              "tcvrLaneToLedId": {

              }
          },
          "37": {
              "tcvrId": 37,
              "accessControl": {
                "controllerId": "accessController-37",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-37",
                "type": 1,
                "devicePath": "/dev/i2c-33"
              },
              "tcvrLaneToLedId": {

              }
          },
          "38": {
              "tcvrId": 38,
              "accessControl": {
                "controllerId": "accessController-38",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-38",
                "type": 1,
                "devicePath": "/dev/i2c-34"
              },
              "tcvrLaneToLedId": {

              }
          },
          "39": {
              "tcvrId": 39,
              "accessControl": {
                "controllerId": "accessController-39",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-39",
                "type": 1,
                "devicePath": "/dev/i2c-35"
              },
              "tcvrLaneToLedId": {

              }
          },
          "40": {
              "tcvrId": 40,
              "accessControl": {
                "controllerId": "accessController-40",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-40",
                "type": 1,
                "devicePath": "/dev/i2c-36"
              },
              "tcvrLaneToLedId": {

              }
          },
          "41": {
              "tcvrId": 41,
              "accessControl": {
                "controllerId": "accessController-41",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-41",
                "type": 1,
                "devicePath": "/dev/i2c-37"
              },
              "tcvrLaneToLedId": {

              }
          },
          "42": {
              "tcvrId": 42,
              "accessControl": {
                "controllerId": "accessController-42",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-42",
                "type": 1,
                "devicePath": "/dev/i2c-38"
              },
              "tcvrLaneToLedId": {

              }
          },
          "43": {
              "tcvrId": 43,
              "accessControl": {
                "controllerId": "accessController-43",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-43",
                "type": 1,
                "devicePath": "/dev/i2c-39"
              },
              "tcvrLaneToLedId": {

              }
          },
          "44": {
              "tcvrId": 44,
              "accessControl": {
                "controllerId": "accessController-44",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-44",
                "type": 1,
                "devicePath": "/dev/i2c-40"
              },
              "tcvrLaneToLedId": {

              }
          },
          "45": {
              "tcvrId": 45,
              "accessControl": {
                "controllerId": "accessController-45",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-45",
                "type": 1,
                "devicePath": "/dev/i2c-41"
              },
              "tcvrLaneToLedId": {

              }
          },
          "46": {
              "tcvrId": 46,
              "accessControl": {
                "controllerId": "accessController-46",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-46",
                "type": 1,
                "devicePath": "/dev/i2c-42"
              },
              "tcvrLaneToLedId": {

              }
          },
          "47": {
              "tcvrId": 47,
              "accessControl": {
                "controllerId": "accessController-47",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-47",
                "type": 1,
                "devicePath": "/dev/i2c-43"
              },
              "tcvrLaneToLedId": {

              }
          },
          "48": {
              "tcvrId": 48,
              "accessControl": {
                "controllerId": "accessController-48",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-48",
                "type": 1,
                "devicePath": "/dev/i2c-44"
              },
              "tcvrLaneToLedId": {

              }
          },
          "49": {
              "tcvrId": 49,
              "accessControl": {
                "controllerId": "accessController-49",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-49",
                "type": 1,
                "devicePath": "/dev/i2c-45"
              },
              "tcvrLaneToLedId": {

              }
          },
          "50": {
              "tcvrId": 50,
              "accessControl": {
                "controllerId": "accessController-50",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-50",
                "type": 1,
                "devicePath": "/dev/i2c-46"
              },
              "tcvrLaneToLedId": {

              }
          },
          "51": {
              "tcvrId": 51,
              "accessControl": {
                "controllerId": "accessController-51",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-51",
                "type": 1,
                "devicePath": "/dev/i2c-47"
              },
              "tcvrLaneToLedId": {

              }
          },
          "52": {
              "tcvrId": 52,
              "accessControl": {
                "controllerId": "accessController-52",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-52",
                "type": 1,
                "devicePath": "/dev/i2c-48"
              },
              "tcvrLaneToLedId": {

              }
          },
          "53": {
              "tcvrId": 53,
              "accessControl": {
                "controllerId": "accessController-53",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-53",
                "type": 1,
                "devicePath": "/dev/i2c-49"
              },
              "tcvrLaneToLedId": {

              }
          },
          "54": {
              "tcvrId": 54,
              "accessControl": {
                "controllerId": "accessController-54",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-54",
                "type": 1,
                "devicePath": "/dev/i2c-50"
              },
              "tcvrLaneToLedId": {

              }
          },
          "55": {
              "tcvrId": 55,
              "accessControl": {
                "controllerId": "accessController-55",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_2",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_2",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-55",
                "type": 1,
                "devicePath": "/dev/i2c-51"
              },
              "tcvrLaneToLedId": {

              }
          },
          "56": {
              "tcvrId": 56,
              "accessControl": {
                "controllerId": "accessController-56",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_reset_2",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0033/cpld_qsfpdd_intr_present_2",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-56",
                "type": 1,
                "devicePath": "/dev/i2c-52"
              },
              "tcvrLaneToLedId": {

              }
          },
          "57": {
              "tcvrId": 57,
              "accessControl": {
                "controllerId": "accessController-57",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-57",
                "type": 1,
                "devicePath": "/dev/i2c-53"
              },
              "tcvrLaneToLedId": {

              }
          },
          "58": {
              "tcvrId": 58,
              "accessControl": {
                "controllerId": "accessController-58",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-58",
                "type": 1,
                "devicePath": "/dev/i2c-54"
              },
              "tcvrLaneToLedId": {

              }
          },
          "59": {
              "tcvrId": 59,
              "accessControl": {
                "controllerId": "accessController-59",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-59",
                "type": 1,
                "devicePath": "/dev/i2c-55"
              },
              "tcvrLaneToLedId": {

              }
          },
          "60": {
              "tcvrId": 60,
              "accessControl": {
                "controllerId": "accessController-60",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-60",
                "type": 1,
                "devicePath": "/dev/i2c-56"
              },
              "tcvrLaneToLedId": {

              }
          },
          "61": {
              "tcvrId": 61,
              "accessControl": {
                "controllerId": "accessController-61",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-61",
                "type": 1,
                "devicePath": "/dev/i2c-57"
              },
              "tcvrLaneToLedId": {

              }
          },
          "62": {
              "tcvrId": 62,
              "accessControl": {
                "controllerId": "accessController-62",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-62",
                "type": 1,
                "devicePath": "/dev/i2c-58"
              },
              "tcvrLaneToLedId": {

              }
          },
          "63": {
              "tcvrId": 63,
              "accessControl": {
                "controllerId": "accessController-63",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-63",
                "type": 1,
                "devicePath": "/dev/i2c-59"
              },
              "tcvrLaneToLedId": {

              }
          },
          "64": {
              "tcvrId": 64,
              "accessControl": {
                "controllerId": "accessController-64",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_0",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-64",
                "type": 1,
                "devicePath": "/dev/i2c-60"
              },
              "tcvrLaneToLedId": {

              }
          },
          "65": {
              "tcvrId": 65,
              "accessControl": {
                "controllerId": "accessController-65",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-65",
                "type": 1,
                "devicePath": "/dev/i2c-61"
              },
              "tcvrLaneToLedId": {

              }
          },
          "66": {
              "tcvrId": 66,
              "accessControl": {
                "controllerId": "accessController-66",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-66",
                "type": 1,
                "devicePath": "/dev/i2c-62"
              },
              "tcvrLaneToLedId": {

              }
          },
          "67": {
              "tcvrId": 67,
              "accessControl": {
                "controllerId": "accessController-67",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-67",
                "type": 1,
                "devicePath": "/dev/i2c-63"
              },
              "tcvrLaneToLedId": {

              }
          },
          "68": {
              "tcvrId": 68,
              "accessControl": {
                "controllerId": "accessController-68",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-68",
                "type": 1,
                "devicePath": "/dev/i2c-64"
              },
              "tcvrLaneToLedId": {

              }
          },
          "69": {
              "tcvrId": 69,
              "accessControl": {
                "controllerId": "accessController-69",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 16,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-69",
                "type": 1,
                "devicePath": "/dev/i2c-65"
              },
              "tcvrLaneToLedId": {

              }
          },
          "70": {
              "tcvrId": 70,
              "accessControl": {
                "controllerId": "accessController-70",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 32,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-70",
                "type": 1,
                "devicePath": "/dev/i2c-66"
              },
              "tcvrLaneToLedId": {

              }
          },
          "71": {
              "tcvrId": 71,
              "accessControl": {
                "controllerId": "accessController-71",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 64,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-71",
                "type": 1,
                "devicePath": "/dev/i2c-67"
              },
              "tcvrLaneToLedId": {

              }
          },
          "72": {
              "tcvrId": 72,
              "accessControl": {
                "controllerId": "accessController-72",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_1",
                  "mask": 128,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-72",
                "type": 1,
                "devicePath": "/dev/i2c-68"
              },
              "tcvrLaneToLedId": {

              }
          },
          "73": {
              "tcvrId": 73,
              "accessControl": {
                "controllerId": "accessController-73",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_2",
                  "mask": 1,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-73",
                "type": 1,
                "devicePath": "/dev/i2c-69"
              },
              "tcvrLaneToLedId": {

              }
          },
          "74": {
              "tcvrId": 74,
              "accessControl": {
                "controllerId": "accessController-74",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_2",
                  "mask": 2,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-74",
                "type": 1,
                "devicePath": "/dev/i2c-70"
              },
              "tcvrLaneToLedId": {

              }
          },
          "75": {
              "tcvrId": 75,
              "accessControl": {
                "controllerId": "accessController-75",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_2",
                  "mask": 4,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_2",
                  "mask": 4,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-75",
                "type": 1,
                "devicePath": "/dev/i2c-71"
              },
              "tcvrLaneToLedId": {

              }
          },
          "76": {
              "tcvrId": 76,
              "accessControl": {
                "controllerId": "accessController-76",
                "type": 1,
                "reset": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_reset_2",
                  "mask": 8,
                  "gpioOffset": 0,
                  "resetHoldHi": 0
                },
                "presence": {
                  "sysfsPath": "/sys/bus/i2c/devices/30-0034/cpld_qsfpdd_intr_present_2",
                  "mask": 8,
                  "gpioOffset": 0,
                  "presentHoldHi": 1
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-76",
                "type": 1,
                "devicePath": "/dev/i2c-72"
              },
              "tcvrLaneToLedId": {

              }
          }
        },
        "phyMapping": {

        },
        "phyIOControllers": {

        },
        "ledMapping": {

        }
    }
  }
}
)";

static BspPlatformMappingThrift buildMeru400biuPlatformMapping() {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(kJsonBspPlatformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

// TODO: Use pre generated bsp platform mapping from cfgr
Meru400biuBspPlatformMapping::Meru400biuBspPlatformMapping()
    : BspPlatformMapping(buildMeru400biuPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
