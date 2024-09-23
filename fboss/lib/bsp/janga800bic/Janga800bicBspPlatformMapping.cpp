// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.h"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_1/xcvr_reset_1",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_1/xcvr_present_1",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "1",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_1"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_2/xcvr_reset_2",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_2/xcvr_present_2",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "2",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_2"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_3/xcvr_reset_3",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_3/xcvr_present_3",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "3",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_3"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_4/xcvr_reset_4",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_4/xcvr_present_4",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "4",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_4"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_5/xcvr_reset_5",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_5/xcvr_present_5",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "5",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_5"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_6/xcvr_reset_6",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_6/xcvr_present_6",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "6",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_6"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_7/xcvr_reset_7",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_7/xcvr_present_7",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "7",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_7"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_8/xcvr_reset_8",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_8/xcvr_present_8",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "8",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_8"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_9/xcvr_reset_9",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_9/xcvr_present_9",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "9",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_9"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_10/xcvr_reset_10",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_10/xcvr_present_10",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "10",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_10"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_11/xcvr_reset_11",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_11/xcvr_present_11",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "11",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_11"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_12/xcvr_reset_12",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_12/xcvr_present_12",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "12",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_12"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_13/xcvr_reset_13",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_13/xcvr_present_13",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "13",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_13"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_14/xcvr_reset_14",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_14/xcvr_present_14",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "14",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_14"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_15/xcvr_reset_15",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_15/xcvr_present_15",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "15",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_15"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_16/xcvr_reset_16",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_16/xcvr_present_16",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "16",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_16"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_17/xcvr_reset_17",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_17/xcvr_present_17",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "17",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_17"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_18/xcvr_reset_18",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_18/xcvr_present_18",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "18",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_18"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_19/xcvr_reset_19",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_19/xcvr_present_19",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "19",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_19"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_20/xcvr_reset_20",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_20/xcvr_present_20",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "20",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_20"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_21/xcvr_reset_21",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_21/xcvr_present_21",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "21",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_21"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_22/xcvr_reset_22",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_22/xcvr_present_22",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "22",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_22"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_23/xcvr_reset_23",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_23/xcvr_present_23",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "23",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_23"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_24/xcvr_reset_24",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_24/xcvr_present_24",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "24",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_24"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_25/xcvr_reset_25",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_25/xcvr_present_25",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "25",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_25"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_26/xcvr_reset_26",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_26/xcvr_present_26",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "26",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_26"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_27/xcvr_reset_27",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_27/xcvr_present_27",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "27",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_27"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_28/xcvr_reset_28",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_28/xcvr_present_28",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "28",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_28"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_29/xcvr_reset_29",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_29/xcvr_present_29",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "29",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_29"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_30/xcvr_reset_30",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_30/xcvr_present_30",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "30",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_30"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_31/xcvr_reset_31",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_31/xcvr_present_31",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "31",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_31"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_32/xcvr_reset_32",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_32/xcvr_present_32",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "32",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_32"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_33/xcvr_reset_33",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_33/xcvr_present_33",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "33",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_33"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_34/xcvr_reset_34",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_34/xcvr_present_34",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "34",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_34"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_35/xcvr_reset_35",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_35/xcvr_present_35",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "35",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_35"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_36/xcvr_reset_36",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_36/xcvr_present_36",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "36",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_36"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_37/xcvr_reset_37",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_37/xcvr_present_37",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "37",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_37"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_38/xcvr_reset_38",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_38/xcvr_present_38",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "38",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_38"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_39/xcvr_reset_39",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_39/xcvr_present_39",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "39",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_39"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_40/xcvr_reset_40",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_40/xcvr_present_40",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "40",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_40"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_41/xcvr_reset_41",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_41/xcvr_present_41",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "41",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_41"
          },
          "tcvrLaneToLedId": {
            "1": 81,
            "2": 81,
            "3": 81,
            "4": 81
          }
        },
        "42": {
          "tcvrId": 42,
          "accessControl": {
            "controllerId": "42",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_42/xcvr_reset_42",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_42/xcvr_present_42",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "42",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_42"
          },
          "tcvrLaneToLedId": {
            "1": 82,
            "2": 82,
            "3": 82,
            "4": 82,
            "5": 83,
            "6": 83,
            "7": 83,
            "8": 83
          }
        },
        "43": {
          "tcvrId": 43,
          "accessControl": {
            "controllerId": "43",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_43/xcvr_reset_43",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_43/xcvr_present_43",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "43",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_43"
          },
          "tcvrLaneToLedId": {
            "1": 84,
            "2": 84,
            "3": 84,
            "4": 84,
            "5": 85,
            "6": 85,
            "7": 85,
            "8": 85
          }
        },
        "44": {
          "tcvrId": 44,
          "accessControl": {
            "controllerId": "44",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_44/xcvr_reset_44",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_44/xcvr_present_44",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "44",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_44"
          },
          "tcvrLaneToLedId": {
            "1": 86,
            "2": 86,
            "3": 86,
            "4": 86
          }
        },
        "45": {
          "tcvrId": 45,
          "accessControl": {
            "controllerId": "45",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_45/xcvr_reset_45",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_45/xcvr_present_45",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "45",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_45"
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
        "46": {
          "tcvrId": 46,
          "accessControl": {
            "controllerId": "46",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_46/xcvr_reset_46",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_46/xcvr_present_46",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "46",
            "type": 1,
            "devicePath": "/run/devmap/i2c-busses/XCVR_46"
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
        }
      },
      "phyMapping": {},
      "phyIOControllers": {},
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port1_led1:blue:status",
          "yellowPath": "/sys/class/leds/port1_led1:yellow:status",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port1_led2:blue:status",
          "yellowPath": "/sys/class/leds/port1_led2:yellow:status",
          "transceiverId": 1
        },
        "3": {
          "id": 3,
          "bluePath": "/sys/class/leds/port2_led1:blue:status",
          "yellowPath": "/sys/class/leds/port2_led1:yellow:status",
          "transceiverId": 2
        },
        "4": {
          "id": 4,
          "bluePath": "/sys/class/leds/port2_led2:blue:status",
          "yellowPath": "/sys/class/leds/port2_led2:yellow:status",
          "transceiverId": 2
        },
        "5": {
          "id": 5,
          "bluePath": "/sys/class/leds/port3_led1:blue:status",
          "yellowPath": "/sys/class/leds/port3_led1:yellow:status",
          "transceiverId": 3
        },
        "6": {
          "id": 6,
          "bluePath": "/sys/class/leds/port3_led2:blue:status",
          "yellowPath": "/sys/class/leds/port3_led2:yellow:status",
          "transceiverId": 3
        },
        "7": {
          "id": 7,
          "bluePath": "/sys/class/leds/port4_led1:blue:status",
          "yellowPath": "/sys/class/leds/port4_led1:yellow:status",
          "transceiverId": 4
        },
        "8": {
          "id": 8,
          "bluePath": "/sys/class/leds/port4_led2:blue:status",
          "yellowPath": "/sys/class/leds/port4_led2:yellow:status",
          "transceiverId": 4
        },
        "9": {
          "id": 9,
          "bluePath": "/sys/class/leds/port5_led1:blue:status",
          "yellowPath": "/sys/class/leds/port5_led1:yellow:status",
          "transceiverId": 5
        },
        "10": {
          "id": 10,
          "bluePath": "/sys/class/leds/port5_led2:blue:status",
          "yellowPath": "/sys/class/leds/port5_led2:yellow:status",
          "transceiverId": 5
        },
        "11": {
          "id": 11,
          "bluePath": "/sys/class/leds/port6_led1:blue:status",
          "yellowPath": "/sys/class/leds/port6_led1:yellow:status",
          "transceiverId": 6
        },
        "12": {
          "id": 12,
          "bluePath": "/sys/class/leds/port6_led2:blue:status",
          "yellowPath": "/sys/class/leds/port6_led2:yellow:status",
          "transceiverId": 6
        },
        "13": {
          "id": 13,
          "bluePath": "/sys/class/leds/port7_led1:blue:status",
          "yellowPath": "/sys/class/leds/port7_led1:yellow:status",
          "transceiverId": 7
        },
        "14": {
          "id": 14,
          "bluePath": "/sys/class/leds/port7_led2:blue:status",
          "yellowPath": "/sys/class/leds/port7_led2:yellow:status",
          "transceiverId": 7
        },
        "15": {
          "id": 15,
          "bluePath": "/sys/class/leds/port8_led1:blue:status",
          "yellowPath": "/sys/class/leds/port8_led1:yellow:status",
          "transceiverId": 8
        },
        "16": {
          "id": 16,
          "bluePath": "/sys/class/leds/port8_led2:blue:status",
          "yellowPath": "/sys/class/leds/port8_led2:yellow:status",
          "transceiverId": 8
        },
        "17": {
          "id": 17,
          "bluePath": "/sys/class/leds/port9_led1:blue:status",
          "yellowPath": "/sys/class/leds/port9_led1:yellow:status",
          "transceiverId": 9
        },
        "18": {
          "id": 18,
          "bluePath": "/sys/class/leds/port9_led2:blue:status",
          "yellowPath": "/sys/class/leds/port9_led2:yellow:status",
          "transceiverId": 9
        },
        "19": {
          "id": 19,
          "bluePath": "/sys/class/leds/port10_led1:blue:status",
          "yellowPath": "/sys/class/leds/port10_led1:yellow:status",
          "transceiverId": 10
        },
        "20": {
          "id": 20,
          "bluePath": "/sys/class/leds/port10_led2:blue:status",
          "yellowPath": "/sys/class/leds/port10_led2:yellow:status",
          "transceiverId": 10
        },
        "21": {
          "id": 21,
          "bluePath": "/sys/class/leds/port11_led1:blue:status",
          "yellowPath": "/sys/class/leds/port11_led1:yellow:status",
          "transceiverId": 11
        },
        "22": {
          "id": 22,
          "bluePath": "/sys/class/leds/port11_led2:blue:status",
          "yellowPath": "/sys/class/leds/port11_led2:yellow:status",
          "transceiverId": 11
        },
        "23": {
          "id": 23,
          "bluePath": "/sys/class/leds/port12_led1:blue:status",
          "yellowPath": "/sys/class/leds/port12_led1:yellow:status",
          "transceiverId": 12
        },
        "24": {
          "id": 24,
          "bluePath": "/sys/class/leds/port12_led2:blue:status",
          "yellowPath": "/sys/class/leds/port12_led2:yellow:status",
          "transceiverId": 12
        },
        "25": {
          "id": 25,
          "bluePath": "/sys/class/leds/port13_led1:blue:status",
          "yellowPath": "/sys/class/leds/port13_led1:yellow:status",
          "transceiverId": 13
        },
        "26": {
          "id": 26,
          "bluePath": "/sys/class/leds/port13_led2:blue:status",
          "yellowPath": "/sys/class/leds/port13_led2:yellow:status",
          "transceiverId": 13
        },
        "27": {
          "id": 27,
          "bluePath": "/sys/class/leds/port14_led1:blue:status",
          "yellowPath": "/sys/class/leds/port14_led1:yellow:status",
          "transceiverId": 14
        },
        "28": {
          "id": 28,
          "bluePath": "/sys/class/leds/port14_led2:blue:status",
          "yellowPath": "/sys/class/leds/port14_led2:yellow:status",
          "transceiverId": 14
        },
        "29": {
          "id": 29,
          "bluePath": "/sys/class/leds/port15_led1:blue:status",
          "yellowPath": "/sys/class/leds/port15_led1:yellow:status",
          "transceiverId": 15
        },
        "30": {
          "id": 30,
          "bluePath": "/sys/class/leds/port15_led2:blue:status",
          "yellowPath": "/sys/class/leds/port15_led2:yellow:status",
          "transceiverId": 15
        },
        "31": {
          "id": 31,
          "bluePath": "/sys/class/leds/port16_led1:blue:status",
          "yellowPath": "/sys/class/leds/port16_led1:yellow:status",
          "transceiverId": 16
        },
        "32": {
          "id": 32,
          "bluePath": "/sys/class/leds/port16_led2:blue:status",
          "yellowPath": "/sys/class/leds/port16_led2:yellow:status",
          "transceiverId": 16
        },
        "33": {
          "id": 33,
          "bluePath": "/sys/class/leds/port17_led1:blue:status",
          "yellowPath": "/sys/class/leds/port17_led1:yellow:status",
          "transceiverId": 17
        },
        "34": {
          "id": 34,
          "bluePath": "/sys/class/leds/port17_led2:blue:status",
          "yellowPath": "/sys/class/leds/port17_led2:yellow:status",
          "transceiverId": 17
        },
        "35": {
          "id": 35,
          "bluePath": "/sys/class/leds/port18_led1:blue:status",
          "yellowPath": "/sys/class/leds/port18_led1:yellow:status",
          "transceiverId": 18
        },
        "36": {
          "id": 36,
          "bluePath": "/sys/class/leds/port18_led2:blue:status",
          "yellowPath": "/sys/class/leds/port18_led2:yellow:status",
          "transceiverId": 18
        },
        "37": {
          "id": 37,
          "bluePath": "/sys/class/leds/port19_led1:blue:status",
          "yellowPath": "/sys/class/leds/port19_led1:yellow:status",
          "transceiverId": 19
        },
        "38": {
          "id": 38,
          "bluePath": "/sys/class/leds/port19_led2:blue:status",
          "yellowPath": "/sys/class/leds/port19_led2:yellow:status",
          "transceiverId": 19
        },
        "39": {
          "id": 39,
          "bluePath": "/sys/class/leds/port20_led1:blue:status",
          "yellowPath": "/sys/class/leds/port20_led1:yellow:status",
          "transceiverId": 20
        },
        "40": {
          "id": 40,
          "bluePath": "/sys/class/leds/port20_led2:blue:status",
          "yellowPath": "/sys/class/leds/port20_led2:yellow:status",
          "transceiverId": 20
        },
        "41": {
          "id": 41,
          "bluePath": "/sys/class/leds/port21_led1:blue:status",
          "yellowPath": "/sys/class/leds/port21_led1:yellow:status",
          "transceiverId": 21
        },
        "42": {
          "id": 42,
          "bluePath": "/sys/class/leds/port21_led2:blue:status",
          "yellowPath": "/sys/class/leds/port21_led2:yellow:status",
          "transceiverId": 21
        },
        "43": {
          "id": 43,
          "bluePath": "/sys/class/leds/port22_led1:blue:status",
          "yellowPath": "/sys/class/leds/port22_led1:yellow:status",
          "transceiverId": 22
        },
        "44": {
          "id": 44,
          "bluePath": "/sys/class/leds/port22_led2:blue:status",
          "yellowPath": "/sys/class/leds/port22_led2:yellow:status",
          "transceiverId": 22
        },
        "45": {
          "id": 45,
          "bluePath": "/sys/class/leds/port23_led1:blue:status",
          "yellowPath": "/sys/class/leds/port23_led1:yellow:status",
          "transceiverId": 23
        },
        "46": {
          "id": 46,
          "bluePath": "/sys/class/leds/port23_led2:blue:status",
          "yellowPath": "/sys/class/leds/port23_led2:yellow:status",
          "transceiverId": 23
        },
        "47": {
          "id": 47,
          "bluePath": "/sys/class/leds/port24_led1:blue:status",
          "yellowPath": "/sys/class/leds/port24_led1:yellow:status",
          "transceiverId": 24
        },
        "48": {
          "id": 48,
          "bluePath": "/sys/class/leds/port24_led2:blue:status",
          "yellowPath": "/sys/class/leds/port24_led2:yellow:status",
          "transceiverId": 24
        },
        "49": {
          "id": 49,
          "bluePath": "/sys/class/leds/port25_led1:blue:status",
          "yellowPath": "/sys/class/leds/port25_led1:yellow:status",
          "transceiverId": 25
        },
        "50": {
          "id": 50,
          "bluePath": "/sys/class/leds/port25_led2:blue:status",
          "yellowPath": "/sys/class/leds/port25_led2:yellow:status",
          "transceiverId": 25
        },
        "51": {
          "id": 51,
          "bluePath": "/sys/class/leds/port26_led1:blue:status",
          "yellowPath": "/sys/class/leds/port26_led1:yellow:status",
          "transceiverId": 26
        },
        "52": {
          "id": 52,
          "bluePath": "/sys/class/leds/port26_led2:blue:status",
          "yellowPath": "/sys/class/leds/port26_led2:yellow:status",
          "transceiverId": 26
        },
        "53": {
          "id": 53,
          "bluePath": "/sys/class/leds/port27_led1:blue:status",
          "yellowPath": "/sys/class/leds/port27_led1:yellow:status",
          "transceiverId": 27
        },
        "54": {
          "id": 54,
          "bluePath": "/sys/class/leds/port27_led2:blue:status",
          "yellowPath": "/sys/class/leds/port27_led2:yellow:status",
          "transceiverId": 27
        },
        "55": {
          "id": 55,
          "bluePath": "/sys/class/leds/port28_led1:blue:status",
          "yellowPath": "/sys/class/leds/port28_led1:yellow:status",
          "transceiverId": 28
        },
        "56": {
          "id": 56,
          "bluePath": "/sys/class/leds/port28_led2:blue:status",
          "yellowPath": "/sys/class/leds/port28_led2:yellow:status",
          "transceiverId": 28
        },
        "57": {
          "id": 57,
          "bluePath": "/sys/class/leds/port29_led1:blue:status",
          "yellowPath": "/sys/class/leds/port29_led1:yellow:status",
          "transceiverId": 29
        },
        "58": {
          "id": 58,
          "bluePath": "/sys/class/leds/port29_led2:blue:status",
          "yellowPath": "/sys/class/leds/port29_led2:yellow:status",
          "transceiverId": 29
        },
        "59": {
          "id": 59,
          "bluePath": "/sys/class/leds/port30_led1:blue:status",
          "yellowPath": "/sys/class/leds/port30_led1:yellow:status",
          "transceiverId": 30
        },
        "60": {
          "id": 60,
          "bluePath": "/sys/class/leds/port30_led2:blue:status",
          "yellowPath": "/sys/class/leds/port30_led2:yellow:status",
          "transceiverId": 30
        },
        "61": {
          "id": 61,
          "bluePath": "/sys/class/leds/port31_led1:blue:status",
          "yellowPath": "/sys/class/leds/port31_led1:yellow:status",
          "transceiverId": 31
        },
        "62": {
          "id": 62,
          "bluePath": "/sys/class/leds/port31_led2:blue:status",
          "yellowPath": "/sys/class/leds/port31_led2:yellow:status",
          "transceiverId": 31
        },
        "63": {
          "id": 63,
          "bluePath": "/sys/class/leds/port32_led1:blue:status",
          "yellowPath": "/sys/class/leds/port32_led1:yellow:status",
          "transceiverId": 32
        },
        "64": {
          "id": 64,
          "bluePath": "/sys/class/leds/port32_led2:blue:status",
          "yellowPath": "/sys/class/leds/port32_led2:yellow:status",
          "transceiverId": 32
        },
        "65": {
          "id": 65,
          "bluePath": "/sys/class/leds/port33_led1:blue:status",
          "yellowPath": "/sys/class/leds/port33_led1:yellow:status",
          "transceiverId": 33
        },
        "66": {
          "id": 66,
          "bluePath": "/sys/class/leds/port33_led2:blue:status",
          "yellowPath": "/sys/class/leds/port33_led2:yellow:status",
          "transceiverId": 33
        },
        "67": {
          "id": 67,
          "bluePath": "/sys/class/leds/port34_led1:blue:status",
          "yellowPath": "/sys/class/leds/port34_led1:yellow:status",
          "transceiverId": 34
        },
        "68": {
          "id": 68,
          "bluePath": "/sys/class/leds/port34_led2:blue:status",
          "yellowPath": "/sys/class/leds/port34_led2:yellow:status",
          "transceiverId": 34
        },
        "69": {
          "id": 69,
          "bluePath": "/sys/class/leds/port35_led1:blue:status",
          "yellowPath": "/sys/class/leds/port35_led1:yellow:status",
          "transceiverId": 35
        },
        "70": {
          "id": 70,
          "bluePath": "/sys/class/leds/port35_led2:blue:status",
          "yellowPath": "/sys/class/leds/port35_led2:yellow:status",
          "transceiverId": 35
        },
        "71": {
          "id": 71,
          "bluePath": "/sys/class/leds/port36_led1:blue:status",
          "yellowPath": "/sys/class/leds/port36_led1:yellow:status",
          "transceiverId": 36
        },
        "72": {
          "id": 72,
          "bluePath": "/sys/class/leds/port36_led2:blue:status",
          "yellowPath": "/sys/class/leds/port36_led2:yellow:status",
          "transceiverId": 36
        },
        "73": {
          "id": 73,
          "bluePath": "/sys/class/leds/port37_led1:blue:status",
          "yellowPath": "/sys/class/leds/port37_led1:yellow:status",
          "transceiverId": 37
        },
        "74": {
          "id": 74,
          "bluePath": "/sys/class/leds/port37_led2:blue:status",
          "yellowPath": "/sys/class/leds/port37_led2:yellow:status",
          "transceiverId": 37
        },
        "75": {
          "id": 75,
          "bluePath": "/sys/class/leds/port38_led1:blue:status",
          "yellowPath": "/sys/class/leds/port38_led1:yellow:status",
          "transceiverId": 38
        },
        "76": {
          "id": 76,
          "bluePath": "/sys/class/leds/port38_led2:blue:status",
          "yellowPath": "/sys/class/leds/port38_led2:yellow:status",
          "transceiverId": 38
        },
        "77": {
          "id": 77,
          "bluePath": "/sys/class/leds/port39_led1:blue:status",
          "yellowPath": "/sys/class/leds/port39_led1:yellow:status",
          "transceiverId": 39
        },
        "78": {
          "id": 78,
          "bluePath": "/sys/class/leds/port39_led2:blue:status",
          "yellowPath": "/sys/class/leds/port39_led2:yellow:status",
          "transceiverId": 39
        },
        "79": {
          "id": 79,
          "bluePath": "/sys/class/leds/port40_led1:blue:status",
          "yellowPath": "/sys/class/leds/port40_led1:yellow:status",
          "transceiverId": 40
        },
        "80": {
          "id": 80,
          "bluePath": "/sys/class/leds/port40_led2:blue:status",
          "yellowPath": "/sys/class/leds/port40_led2:yellow:status",
          "transceiverId": 40
        },
        "81": {
          "id": 81,
          "bluePath": "/sys/class/leds/port41_led1:blue:status",
          "yellowPath": "/sys/class/leds/port41_led1:yellow:status",
          "transceiverId": 41
        },
        "82": {
          "id": 82,
          "bluePath": "/sys/class/leds/port42_led1:blue:status",
          "yellowPath": "/sys/class/leds/port42_led1:yellow:status",
          "transceiverId": 42
        },
        "83": {
          "id": 83,
          "bluePath": "/sys/class/leds/port42_led2:blue:status",
          "yellowPath": "/sys/class/leds/port42_led2:yellow:status",
          "transceiverId": 42
        },
        "84": {
          "id": 84,
          "bluePath": "/sys/class/leds/port43_led1:blue:status",
          "yellowPath": "/sys/class/leds/port43_led1:yellow:status",
          "transceiverId": 43
        },
        "85": {
          "id": 85,
          "bluePath": "/sys/class/leds/port43_led2:blue:status",
          "yellowPath": "/sys/class/leds/port43_led2:yellow:status",
          "transceiverId": 43
        },
        "86": {
          "id": 86,
          "bluePath": "/sys/class/leds/port44_led1:blue:status",
          "yellowPath": "/sys/class/leds/port44_led1:yellow:status",
          "transceiverId": 44
        },
        "87": {
          "id": 87,
          "bluePath": "/sys/class/leds/port45_led1:blue:status",
          "yellowPath": "/sys/class/leds/port45_led1:yellow:status",
          "transceiverId": 45
        },
        "88": {
          "id": 88,
          "bluePath": "/sys/class/leds/port45_led2:blue:status",
          "yellowPath": "/sys/class/leds/port45_led2:yellow:status",
          "transceiverId": 45
        },
        "89": {
          "id": 89,
          "bluePath": "/sys/class/leds/port46_led1:blue:status",
          "yellowPath": "/sys/class/leds/port46_led1:yellow:status",
          "transceiverId": 46
        },
        "90": {
          "id": 90,
          "bluePath": "/sys/class/leds/port46_led2:blue:status",
          "yellowPath": "/sys/class/leds/port46_led2:yellow:status",
          "transceiverId": 46
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildJanga800bicPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Janga800bicBspPlatformMapping::Janga800bicBspPlatformMapping()
    : BspPlatformMapping(
          buildJanga800bicPlatformMapping(kJsonBspPlatformMappingStr)) {}

Janga800bicBspPlatformMapping::Janga800bicBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildJanga800bicPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
