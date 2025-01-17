// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr1_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr1_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "1",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_1"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_2/xcvr2_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_2/xcvr2_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "2",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_2"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr3_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr3_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "3",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_3"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_4/xcvr4_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_4/xcvr4_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "4",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_4"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_5/xcvr5_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_5/xcvr5_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "5",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_5"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_6/xcvr6_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_6/xcvr6_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "6",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_6"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_7/xcvr7_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_7/xcvr7_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "7",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_7"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_8/xcvr8_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_8/xcvr8_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "8",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_8"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_9/xcvr9_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_9/xcvr9_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "9",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_9"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_10/xcvr10_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_10/xcvr10_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "10",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_10"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_11/xcvr11_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_11/xcvr11_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "11",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_11"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_12/xcvr12_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_12/xcvr12_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "12",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_12"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_13/xcvr13_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_13/xcvr13_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "13",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_13"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_14/xcvr14_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_14/xcvr14_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "14",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_14"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_15/xcvr15_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_15/xcvr15_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "15",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_15"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_16/xcvr16_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_16/xcvr16_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "16",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_16"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_17/xcvr17_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_17/xcvr17_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "17",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_17"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_18/xcvr18_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_18/xcvr18_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "18",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_18"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_19/xcvr19_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_19/xcvr19_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "19",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_19"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_20/xcvr20_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_20/xcvr20_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "20",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_20"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_21/xcvr21_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_21/xcvr21_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "21",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_21"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_22/xcvr22_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_22/xcvr22_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "22",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_22"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_23/xcvr23_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_23/xcvr23_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "23",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_23"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_24/xcvr24_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_24/xcvr24_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "24",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_24"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_25/xcvr25_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_25/xcvr25_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "25",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_25"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_26/xcvr26_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_26/xcvr26_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "26",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_26"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_27/xcvr27_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_27/xcvr27_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "27",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_27"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_28/xcvr28_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_28/xcvr28_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "28",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_28"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_29/xcvr29_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_29/xcvr29_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "29",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_29"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_30/xcvr30_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_30/xcvr30_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "30",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_30"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_31/xcvr31_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_31/xcvr31_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "31",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_31"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_32/xcvr32_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_32/xcvr32_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "32",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_32"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_33/xcvr33_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_33/xcvr33_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "33",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_33"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_34/xcvr34_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_34/xcvr34_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "34",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_34"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_35/xcvr35_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_35/xcvr35_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "35",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_35"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_36/xcvr36_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_36/xcvr36_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "36",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_36"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_37/xcvr37_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_37/xcvr37_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "37",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_37"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_38/xcvr38_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_38/xcvr38_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "38",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_38"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_39/xcvr39_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_39/xcvr39_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "39",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_39"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_40/xcvr40_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_40/xcvr40_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "40",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_40"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_41/xcvr41_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_41/xcvr41_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "41",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_41"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_42/xcvr42_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_42/xcvr42_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "42",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_42"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_43/xcvr43_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_43/xcvr43_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "43",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_43"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_44/xcvr44_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_44/xcvr44_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "44",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_44"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_45/xcvr45_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_45/xcvr45_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "45",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_45"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_46/xcvr46_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_46/xcvr46_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "46",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_46"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_47/xcvr47_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_47/xcvr47_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "47",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_47"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_48/xcvr48_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_48/xcvr48_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "48",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_48"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_49/xcvr49_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_49/xcvr49_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "49",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_49"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_50/xcvr50_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_50/xcvr50_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "50",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_50"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_51/xcvr51_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_51/xcvr51_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "51",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_51"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_52/xcvr52_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_52/xcvr52_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "52",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_52"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_53/xcvr53_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_53/xcvr53_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "53",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_53"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_54/xcvr54_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_54/xcvr54_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "54",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_54"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_55/xcvr55_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_55/xcvr55_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "55",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_55"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_56/xcvr56_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_56/xcvr56_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "56",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_56"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_57/xcvr57_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_57/xcvr57_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "57",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_57"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_58/xcvr58_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_58/xcvr58_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "58",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_58"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_59/xcvr59_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_59/xcvr59_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "59",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_59"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_60/xcvr60_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_60/xcvr60_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "60",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_60"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_61/xcvr61_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_61/xcvr61_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "61",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_61"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_62/xcvr62_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_62/xcvr62_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "62",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_62"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_63/xcvr63_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_63/xcvr63_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "63",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_63"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_64/xcvr64_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_64/xcvr64_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "64",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_64"
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
        },
        "65": {
          "tcvrId": 65,
          "accessControl": {
            "controllerId": "65",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_65/xcvr65_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_65/xcvr65_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "65",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_65"
          },
          "tcvrLaneToLedId": {
            "1": 129,
            "2": 129,
            "3": 129,
            "4": 129,
            "5": 130,
            "6": 130,
            "7": 130,
            "8": 130
          }
        },
        "66": {
          "tcvrId": 66,
          "accessControl": {
            "controllerId": "66",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_66/xcvr66_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_66/xcvr66_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "66",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_66"
          },
          "tcvrLaneToLedId": {
            "1": 131,
            "2": 131,
            "3": 131,
            "4": 131,
            "5": 132,
            "6": 132,
            "7": 132,
            "8": 132
          }
        },
        "67": {
          "tcvrId": 67,
          "accessControl": {
            "controllerId": "67",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_67/xcvr67_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_67/xcvr67_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "67",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_67"
          },
          "tcvrLaneToLedId": {
            "1": 133,
            "2": 133,
            "3": 133,
            "4": 133,
            "5": 134,
            "6": 134,
            "7": 134,
            "8": 134
          }
        },
        "68": {
          "tcvrId": 68,
          "accessControl": {
            "controllerId": "68",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_68/xcvr68_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_68/xcvr68_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "68",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_68"
          },
          "tcvrLaneToLedId": {
            "1": 135,
            "2": 135,
            "3": 135,
            "4": 135,
            "5": 136,
            "6": 136,
            "7": 136,
            "8": 136
          }
        },
        "69": {
          "tcvrId": 69,
          "accessControl": {
            "controllerId": "69",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_69/xcvr69_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_69/xcvr69_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "69",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_69"
          },
          "tcvrLaneToLedId": {
            "1": 137,
            "2": 137,
            "3": 137,
            "4": 137,
            "5": 138,
            "6": 138,
            "7": 138,
            "8": 138
          }
        },
        "70": {
          "tcvrId": 70,
          "accessControl": {
            "controllerId": "70",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_70/xcvr70_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_70/xcvr70_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "70",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_70"
          },
          "tcvrLaneToLedId": {
            "1": 139,
            "2": 139,
            "3": 139,
            "4": 139,
            "5": 140,
            "6": 140,
            "7": 140,
            "8": 140
          }
        },
        "71": {
          "tcvrId": 71,
          "accessControl": {
            "controllerId": "71",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_71/xcvr71_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_71/xcvr71_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "71",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_71"
          },
          "tcvrLaneToLedId": {
            "1": 141,
            "2": 141,
            "3": 141,
            "4": 141,
            "5": 142,
            "6": 142,
            "7": 142,
            "8": 142
          }
        },
        "72": {
          "tcvrId": 72,
          "accessControl": {
            "controllerId": "72",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_72/xcvr72_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_72/xcvr72_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "72",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_72"
          },
          "tcvrLaneToLedId": {
            "1": 143,
            "2": 143,
            "3": 143,
            "4": 143,
            "5": 144,
            "6": 144,
            "7": 144,
            "8": 144
          }
        },
        "73": {
          "tcvrId": 73,
          "accessControl": {
            "controllerId": "73",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_73/xcvr73_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_73/xcvr73_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "73",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_73"
          },
          "tcvrLaneToLedId": {
            "1": 145,
            "2": 145,
            "3": 145,
            "4": 145,
            "5": 146,
            "6": 146,
            "7": 146,
            "8": 146
          }
        },
        "74": {
          "tcvrId": 74,
          "accessControl": {
            "controllerId": "74",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_74/xcvr74_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_74/xcvr74_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "74",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_74"
          },
          "tcvrLaneToLedId": {
            "1": 147,
            "2": 147,
            "3": 147,
            "4": 147,
            "5": 148,
            "6": 148,
            "7": 148,
            "8": 148
          }
        },
        "75": {
          "tcvrId": 75,
          "accessControl": {
            "controllerId": "75",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_75/xcvr75_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_75/xcvr75_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "75",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_75"
          },
          "tcvrLaneToLedId": {
            "1": 149,
            "2": 149,
            "3": 149,
            "4": 149,
            "5": 150,
            "6": 150,
            "7": 150,
            "8": 150
          }
        },
        "76": {
          "tcvrId": 76,
          "accessControl": {
            "controllerId": "76",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_76/xcvr76_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_76/xcvr76_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "76",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_76"
          },
          "tcvrLaneToLedId": {
            "1": 151,
            "2": 151,
            "3": 151,
            "4": 151,
            "5": 152,
            "6": 152,
            "7": 152,
            "8": 152
          }
        },
        "77": {
          "tcvrId": 77,
          "accessControl": {
            "controllerId": "77",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_77/xcvr77_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_77/xcvr77_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "77",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_77"
          },
          "tcvrLaneToLedId": {
            "1": 153,
            "2": 153,
            "3": 153,
            "4": 153,
            "5": 154,
            "6": 154,
            "7": 154,
            "8": 154
          }
        },
        "78": {
          "tcvrId": 78,
          "accessControl": {
            "controllerId": "78",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_78/xcvr78_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_78/xcvr78_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "78",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_78"
          },
          "tcvrLaneToLedId": {
            "1": 155,
            "2": 155,
            "3": 155,
            "4": 155,
            "5": 156,
            "6": 156,
            "7": 156,
            "8": 156
          }
        },
        "79": {
          "tcvrId": 79,
          "accessControl": {
            "controllerId": "79",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_79/xcvr79_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_79/xcvr79_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "79",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_79"
          },
          "tcvrLaneToLedId": {
            "1": 157,
            "2": 157,
            "3": 157,
            "4": 157,
            "5": 158,
            "6": 158,
            "7": 158,
            "8": 158
          }
        },
        "80": {
          "tcvrId": 80,
          "accessControl": {
            "controllerId": "80",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_80/xcvr80_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_80/xcvr80_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "80",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_80"
          },
          "tcvrLaneToLedId": {
            "1": 159,
            "2": 159,
            "3": 159,
            "4": 159,
            "5": 160,
            "6": 160,
            "7": 160,
            "8": 160
          }
        },
        "81": {
          "tcvrId": 81,
          "accessControl": {
            "controllerId": "81",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_81/xcvr81_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_81/xcvr81_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "81",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_81"
          },
          "tcvrLaneToLedId": {
            "1": 161,
            "2": 161,
            "3": 161,
            "4": 161,
            "5": 162,
            "6": 162,
            "7": 162,
            "8": 162
          }
        },
        "82": {
          "tcvrId": 82,
          "accessControl": {
            "controllerId": "82",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_82/xcvr82_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_82/xcvr82_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "82",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_82"
          },
          "tcvrLaneToLedId": {
            "1": 163,
            "2": 163,
            "3": 163,
            "4": 163,
            "5": 164,
            "6": 164,
            "7": 164,
            "8": 164
          }
        },
        "83": {
          "tcvrId": 83,
          "accessControl": {
            "controllerId": "83",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_83/xcvr83_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_83/xcvr83_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "83",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_83"
          },
          "tcvrLaneToLedId": {
            "1": 165,
            "2": 165,
            "3": 165,
            "4": 165,
            "5": 166,
            "6": 166,
            "7": 166,
            "8": 166
          }
        },
        "84": {
          "tcvrId": 84,
          "accessControl": {
            "controllerId": "84",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_84/xcvr84_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_84/xcvr84_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "84",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_84"
          },
          "tcvrLaneToLedId": {
            "1": 167,
            "2": 167,
            "3": 167,
            "4": 167,
            "5": 168,
            "6": 168,
            "7": 168,
            "8": 168
          }
        },
        "85": {
          "tcvrId": 85,
          "accessControl": {
            "controllerId": "85",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_85/xcvr85_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_85/xcvr85_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "85",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_85"
          },
          "tcvrLaneToLedId": {
            "1": 169,
            "2": 169,
            "3": 169,
            "4": 169,
            "5": 170,
            "6": 170,
            "7": 170,
            "8": 170
          }
        },
        "86": {
          "tcvrId": 86,
          "accessControl": {
            "controllerId": "86",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_86/xcvr86_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_86/xcvr86_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "86",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_86"
          },
          "tcvrLaneToLedId": {
            "1": 171,
            "2": 171,
            "3": 171,
            "4": 171,
            "5": 172,
            "6": 172,
            "7": 172,
            "8": 172
          }
        },
        "87": {
          "tcvrId": 87,
          "accessControl": {
            "controllerId": "87",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_87/xcvr87_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_87/xcvr87_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "87",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_87"
          },
          "tcvrLaneToLedId": {
            "1": 173,
            "2": 173,
            "3": 173,
            "4": 173,
            "5": 174,
            "6": 174,
            "7": 174,
            "8": 174
          }
        },
        "88": {
          "tcvrId": 88,
          "accessControl": {
            "controllerId": "88",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_88/xcvr88_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_88/xcvr88_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "88",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_88"
          },
          "tcvrLaneToLedId": {
            "1": 175,
            "2": 175,
            "3": 175,
            "4": 175,
            "5": 176,
            "6": 176,
            "7": 176,
            "8": 176
          }
        },
        "89": {
          "tcvrId": 89,
          "accessControl": {
            "controllerId": "89",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_89/xcvr89_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_89/xcvr89_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "89",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_89"
          },
          "tcvrLaneToLedId": {
            "1": 177,
            "2": 177,
            "3": 177,
            "4": 177,
            "5": 178,
            "6": 178,
            "7": 178,
            "8": 178
          }
        },
        "90": {
          "tcvrId": 90,
          "accessControl": {
            "controllerId": "90",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_90/xcvr90_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_90/xcvr90_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "90",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_90"
          },
          "tcvrLaneToLedId": {
            "1": 179,
            "2": 179,
            "3": 179,
            "4": 179,
            "5": 180,
            "6": 180,
            "7": 180,
            "8": 180
          }
        },
        "91": {
          "tcvrId": 91,
          "accessControl": {
            "controllerId": "91",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_91/xcvr91_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_91/xcvr91_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "91",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_91"
          },
          "tcvrLaneToLedId": {
            "1": 181,
            "2": 181,
            "3": 181,
            "4": 181,
            "5": 182,
            "6": 182,
            "7": 182,
            "8": 182
          }
        },
        "92": {
          "tcvrId": 92,
          "accessControl": {
            "controllerId": "92",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_92/xcvr92_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_92/xcvr92_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "92",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_92"
          },
          "tcvrLaneToLedId": {
            "1": 183,
            "2": 183,
            "3": 183,
            "4": 183,
            "5": 184,
            "6": 184,
            "7": 184,
            "8": 184
          }
        },
        "93": {
          "tcvrId": 93,
          "accessControl": {
            "controllerId": "93",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_93/xcvr93_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_93/xcvr93_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "93",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_93"
          },
          "tcvrLaneToLedId": {
            "1": 185,
            "2": 185,
            "3": 185,
            "4": 185,
            "5": 186,
            "6": 186,
            "7": 186,
            "8": 186
          }
        },
        "94": {
          "tcvrId": 94,
          "accessControl": {
            "controllerId": "94",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_94/xcvr94_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_94/xcvr94_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "94",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_94"
          },
          "tcvrLaneToLedId": {
            "1": 187,
            "2": 187,
            "3": 187,
            "4": 187,
            "5": 188,
            "6": 188,
            "7": 188,
            "8": 188
          }
        },
        "95": {
          "tcvrId": 95,
          "accessControl": {
            "controllerId": "95",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_95/xcvr95_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_95/xcvr95_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "95",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_95"
          },
          "tcvrLaneToLedId": {
            "1": 189,
            "2": 189,
            "3": 189,
            "4": 189,
            "5": 190,
            "6": 190,
            "7": 190,
            "8": 190
          }
        },
        "96": {
          "tcvrId": 96,
          "accessControl": {
            "controllerId": "96",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_96/xcvr96_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_96/xcvr96_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "96",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_96"
          },
          "tcvrLaneToLedId": {
            "1": 191,
            "2": 191,
            "3": 191,
            "4": 191,
            "5": 192,
            "6": 192,
            "7": 192,
            "8": 192
          }
        },
        "97": {
          "tcvrId": 97,
          "accessControl": {
            "controllerId": "97",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_97/xcvr97_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_97/xcvr97_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "97",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_97"
          },
          "tcvrLaneToLedId": {
            "1": 193,
            "2": 193,
            "3": 193,
            "4": 193,
            "5": 194,
            "6": 194,
            "7": 194,
            "8": 194
          }
        },
        "98": {
          "tcvrId": 98,
          "accessControl": {
            "controllerId": "98",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_98/xcvr98_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_98/xcvr98_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "98",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_98"
          },
          "tcvrLaneToLedId": {
            "1": 195,
            "2": 195,
            "3": 195,
            "4": 195,
            "5": 196,
            "6": 196,
            "7": 196,
            "8": 196
          }
        },
        "99": {
          "tcvrId": 99,
          "accessControl": {
            "controllerId": "99",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_99/xcvr99_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_99/xcvr99_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "99",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_99"
          },
          "tcvrLaneToLedId": {
            "1": 197,
            "2": 197,
            "3": 197,
            "4": 197,
            "5": 198,
            "6": 198,
            "7": 198,
            "8": 198
          }
        },
        "100": {
          "tcvrId": 100,
          "accessControl": {
            "controllerId": "100",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_100/xcvr100_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_100/xcvr100_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "100",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_100"
          },
          "tcvrLaneToLedId": {
            "1": 199,
            "2": 199,
            "3": 199,
            "4": 199,
            "5": 200,
            "6": 200,
            "7": 200,
            "8": 200
          }
        },
        "101": {
          "tcvrId": 101,
          "accessControl": {
            "controllerId": "101",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_101/xcvr101_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_101/xcvr101_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "101",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_101"
          },
          "tcvrLaneToLedId": {
            "1": 201,
            "2": 201,
            "3": 201,
            "4": 201,
            "5": 202,
            "6": 202,
            "7": 202,
            "8": 202
          }
        },
        "102": {
          "tcvrId": 102,
          "accessControl": {
            "controllerId": "102",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_102/xcvr102_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_102/xcvr102_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "102",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_102"
          },
          "tcvrLaneToLedId": {
            "1": 203,
            "2": 203,
            "3": 203,
            "4": 203,
            "5": 204,
            "6": 204,
            "7": 204,
            "8": 204
          }
        },
        "103": {
          "tcvrId": 103,
          "accessControl": {
            "controllerId": "103",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_103/xcvr103_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_103/xcvr103_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "103",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_103"
          },
          "tcvrLaneToLedId": {
            "1": 205,
            "2": 205,
            "3": 205,
            "4": 205,
            "5": 206,
            "6": 206,
            "7": 206,
            "8": 206
          }
        },
        "104": {
          "tcvrId": 104,
          "accessControl": {
            "controllerId": "104",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_104/xcvr104_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_104/xcvr104_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "104",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_104"
          },
          "tcvrLaneToLedId": {
            "1": 207,
            "2": 207,
            "3": 207,
            "4": 207,
            "5": 208,
            "6": 208,
            "7": 208,
            "8": 208
          }
        },
        "105": {
          "tcvrId": 105,
          "accessControl": {
            "controllerId": "105",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_105/xcvr105_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_105/xcvr105_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "105",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_105"
          },
          "tcvrLaneToLedId": {
            "1": 209,
            "2": 209,
            "3": 209,
            "4": 209,
            "5": 210,
            "6": 210,
            "7": 210,
            "8": 210
          }
        },
        "106": {
          "tcvrId": 106,
          "accessControl": {
            "controllerId": "106",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_106/xcvr106_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_106/xcvr106_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "106",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_106"
          },
          "tcvrLaneToLedId": {
            "1": 211,
            "2": 211,
            "3": 211,
            "4": 211,
            "5": 212,
            "6": 212,
            "7": 212,
            "8": 212
          }
        },
        "107": {
          "tcvrId": 107,
          "accessControl": {
            "controllerId": "107",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_107/xcvr107_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_107/xcvr107_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "107",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_107"
          },
          "tcvrLaneToLedId": {
            "1": 213,
            "2": 213,
            "3": 213,
            "4": 213,
            "5": 214,
            "6": 214,
            "7": 214,
            "8": 214
          }
        },
        "108": {
          "tcvrId": 108,
          "accessControl": {
            "controllerId": "108",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_108/xcvr108_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_108/xcvr108_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "108",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_108"
          },
          "tcvrLaneToLedId": {
            "1": 215,
            "2": 215,
            "3": 215,
            "4": 215,
            "5": 216,
            "6": 216,
            "7": 216,
            "8": 216
          }
        },
        "109": {
          "tcvrId": 109,
          "accessControl": {
            "controllerId": "109",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_109/xcvr109_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_109/xcvr109_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "109",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_109"
          },
          "tcvrLaneToLedId": {
            "1": 217,
            "2": 217,
            "3": 217,
            "4": 217,
            "5": 218,
            "6": 218,
            "7": 218,
            "8": 218
          }
        },
        "110": {
          "tcvrId": 110,
          "accessControl": {
            "controllerId": "110",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_110/xcvr110_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_110/xcvr110_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "110",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_110"
          },
          "tcvrLaneToLedId": {
            "1": 219,
            "2": 219,
            "3": 219,
            "4": 219,
            "5": 220,
            "6": 220,
            "7": 220,
            "8": 220
          }
        },
        "111": {
          "tcvrId": 111,
          "accessControl": {
            "controllerId": "111",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_111/xcvr111_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_111/xcvr111_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "111",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_111"
          },
          "tcvrLaneToLedId": {
            "1": 221,
            "2": 221,
            "3": 221,
            "4": 221,
            "5": 222,
            "6": 222,
            "7": 222,
            "8": 222
          }
        },
        "112": {
          "tcvrId": 112,
          "accessControl": {
            "controllerId": "112",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_112/xcvr112_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_112/xcvr112_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "112",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_112"
          },
          "tcvrLaneToLedId": {
            "1": 223,
            "2": 223,
            "3": 223,
            "4": 223,
            "5": 224,
            "6": 224,
            "7": 224,
            "8": 224
          }
        },
        "113": {
          "tcvrId": 113,
          "accessControl": {
            "controllerId": "113",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_113/xcvr113_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_113/xcvr113_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "113",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_113"
          },
          "tcvrLaneToLedId": {
            "1": 225,
            "2": 225,
            "3": 225,
            "4": 225,
            "5": 226,
            "6": 226,
            "7": 226,
            "8": 226
          }
        },
        "114": {
          "tcvrId": 114,
          "accessControl": {
            "controllerId": "114",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_114/xcvr114_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_114/xcvr114_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "114",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_114"
          },
          "tcvrLaneToLedId": {
            "1": 227,
            "2": 227,
            "3": 227,
            "4": 227,
            "5": 228,
            "6": 228,
            "7": 228,
            "8": 228
          }
        },
        "115": {
          "tcvrId": 115,
          "accessControl": {
            "controllerId": "115",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_115/xcvr115_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_115/xcvr115_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "115",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_115"
          },
          "tcvrLaneToLedId": {
            "1": 229,
            "2": 229,
            "3": 229,
            "4": 229,
            "5": 230,
            "6": 230,
            "7": 230,
            "8": 230
          }
        },
        "116": {
          "tcvrId": 116,
          "accessControl": {
            "controllerId": "116",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_116/xcvr116_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_116/xcvr116_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "116",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_116"
          },
          "tcvrLaneToLedId": {
            "1": 231,
            "2": 231,
            "3": 231,
            "4": 231,
            "5": 232,
            "6": 232,
            "7": 232,
            "8": 232
          }
        },
        "117": {
          "tcvrId": 117,
          "accessControl": {
            "controllerId": "117",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_117/xcvr117_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_117/xcvr117_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "117",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_117"
          },
          "tcvrLaneToLedId": {
            "1": 233,
            "2": 233,
            "3": 233,
            "4": 233,
            "5": 234,
            "6": 234,
            "7": 234,
            "8": 234
          }
        },
        "118": {
          "tcvrId": 118,
          "accessControl": {
            "controllerId": "118",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_118/xcvr118_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_118/xcvr118_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "118",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_118"
          },
          "tcvrLaneToLedId": {
            "1": 235,
            "2": 235,
            "3": 235,
            "4": 235,
            "5": 236,
            "6": 236,
            "7": 236,
            "8": 236
          }
        },
        "119": {
          "tcvrId": 119,
          "accessControl": {
            "controllerId": "119",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_119/xcvr119_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_119/xcvr119_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "119",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_119"
          },
          "tcvrLaneToLedId": {
            "1": 237,
            "2": 237,
            "3": 237,
            "4": 237,
            "5": 238,
            "6": 238,
            "7": 238,
            "8": 238
          }
        },
        "120": {
          "tcvrId": 120,
          "accessControl": {
            "controllerId": "120",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_120/xcvr120_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_120/xcvr120_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "120",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_120"
          },
          "tcvrLaneToLedId": {
            "1": 239,
            "2": 239,
            "3": 239,
            "4": 239,
            "5": 240,
            "6": 240,
            "7": 240,
            "8": 240
          }
        },
        "121": {
          "tcvrId": 121,
          "accessControl": {
            "controllerId": "121",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_121/xcvr121_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_121/xcvr121_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "121",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_121"
          },
          "tcvrLaneToLedId": {
            "1": 241,
            "2": 241,
            "3": 241,
            "4": 241,
            "5": 242,
            "6": 242,
            "7": 242,
            "8": 242
          }
        },
        "122": {
          "tcvrId": 122,
          "accessControl": {
            "controllerId": "122",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_122/xcvr122_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_122/xcvr122_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "122",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_122"
          },
          "tcvrLaneToLedId": {
            "1": 243,
            "2": 243,
            "3": 243,
            "4": 243,
            "5": 244,
            "6": 244,
            "7": 244,
            "8": 244
          }
        },
        "123": {
          "tcvrId": 123,
          "accessControl": {
            "controllerId": "123",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_123/xcvr123_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_123/xcvr123_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "123",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_123"
          },
          "tcvrLaneToLedId": {
            "1": 245,
            "2": 245,
            "3": 245,
            "4": 245,
            "5": 246,
            "6": 246,
            "7": 246,
            "8": 246
          }
        },
        "124": {
          "tcvrId": 124,
          "accessControl": {
            "controllerId": "124",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_124/xcvr124_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_124/xcvr124_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "124",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_124"
          },
          "tcvrLaneToLedId": {
            "1": 247,
            "2": 247,
            "3": 247,
            "4": 247,
            "5": 248,
            "6": 248,
            "7": 248,
            "8": 248
          }
        },
        "125": {
          "tcvrId": 125,
          "accessControl": {
            "controllerId": "125",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_125/xcvr125_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_125/xcvr125_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "125",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_125"
          },
          "tcvrLaneToLedId": {
            "1": 249,
            "2": 249,
            "3": 249,
            "4": 249,
            "5": 250,
            "6": 250,
            "7": 250,
            "8": 250
          }
        },
        "126": {
          "tcvrId": 126,
          "accessControl": {
            "controllerId": "126",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_126/xcvr126_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_126/xcvr126_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "126",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_126"
          },
          "tcvrLaneToLedId": {
            "1": 251,
            "2": 251,
            "3": 251,
            "4": 251,
            "5": 252,
            "6": 252,
            "7": 252,
            "8": 252
          }
        },
        "127": {
          "tcvrId": 127,
          "accessControl": {
            "controllerId": "127",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_127/xcvr127_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_127/xcvr127_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "127",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_127"
          },
          "tcvrLaneToLedId": {
            "1": 253,
            "2": 253,
            "3": 253,
            "4": 253,
            "5": 254,
            "6": 254,
            "7": 254,
            "8": 254
          }
        },
        "128": {
          "tcvrId": 128,
          "accessControl": {
            "controllerId": "128",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_128/xcvr128_reset",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_128/xcvr128_present",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "128",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_128"
          },
          "tcvrLaneToLedId": {
            "1": 255,
            "2": 255,
            "3": 255,
            "4": 255,
            "5": 256,
            "6": 256,
            "7": 256,
            "8": 256
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
          "bluePath": "/sys/class/leds/port41_led2:blue:status",
          "yellowPath": "/sys/class/leds/port41_led2:yellow:status",
          "transceiverId": 41
        },
        "83": {
          "id": 83,
          "bluePath": "/sys/class/leds/port42_led1:blue:status",
          "yellowPath": "/sys/class/leds/port42_led1:yellow:status",
          "transceiverId": 42
        },
        "84": {
          "id": 84,
          "bluePath": "/sys/class/leds/port42_led2:blue:status",
          "yellowPath": "/sys/class/leds/port42_led2:yellow:status",
          "transceiverId": 42
        },
        "85": {
          "id": 85,
          "bluePath": "/sys/class/leds/port43_led1:blue:status",
          "yellowPath": "/sys/class/leds/port43_led1:yellow:status",
          "transceiverId": 43
        },
        "86": {
          "id": 86,
          "bluePath": "/sys/class/leds/port43_led2:blue:status",
          "yellowPath": "/sys/class/leds/port43_led2:yellow:status",
          "transceiverId": 43
        },
        "87": {
          "id": 87,
          "bluePath": "/sys/class/leds/port44_led1:blue:status",
          "yellowPath": "/sys/class/leds/port44_led1:yellow:status",
          "transceiverId": 44
        },
        "88": {
          "id": 88,
          "bluePath": "/sys/class/leds/port44_led2:blue:status",
          "yellowPath": "/sys/class/leds/port44_led2:yellow:status",
          "transceiverId": 44
        },
        "89": {
          "id": 89,
          "bluePath": "/sys/class/leds/port45_led1:blue:status",
          "yellowPath": "/sys/class/leds/port45_led1:yellow:status",
          "transceiverId": 45
        },
        "90": {
          "id": 90,
          "bluePath": "/sys/class/leds/port45_led2:blue:status",
          "yellowPath": "/sys/class/leds/port45_led2:yellow:status",
          "transceiverId": 45
        },
        "91": {
          "id": 91,
          "bluePath": "/sys/class/leds/port46_led1:blue:status",
          "yellowPath": "/sys/class/leds/port46_led1:yellow:status",
          "transceiverId": 46
        },
        "92": {
          "id": 92,
          "bluePath": "/sys/class/leds/port46_led2:blue:status",
          "yellowPath": "/sys/class/leds/port46_led2:yellow:status",
          "transceiverId": 46
        },
        "93": {
          "id": 93,
          "bluePath": "/sys/class/leds/port47_led1:blue:status",
          "yellowPath": "/sys/class/leds/port47_led1:yellow:status",
          "transceiverId": 47
        },
        "94": {
          "id": 94,
          "bluePath": "/sys/class/leds/port47_led2:blue:status",
          "yellowPath": "/sys/class/leds/port47_led2:yellow:status",
          "transceiverId": 47
        },
        "95": {
          "id": 95,
          "bluePath": "/sys/class/leds/port48_led1:blue:status",
          "yellowPath": "/sys/class/leds/port48_led1:yellow:status",
          "transceiverId": 48
        },
        "96": {
          "id": 96,
          "bluePath": "/sys/class/leds/port48_led2:blue:status",
          "yellowPath": "/sys/class/leds/port48_led2:yellow:status",
          "transceiverId": 48
        },
        "97": {
          "id": 97,
          "bluePath": "/sys/class/leds/port49_led1:blue:status",
          "yellowPath": "/sys/class/leds/port49_led1:yellow:status",
          "transceiverId": 49
        },
        "98": {
          "id": 98,
          "bluePath": "/sys/class/leds/port49_led2:blue:status",
          "yellowPath": "/sys/class/leds/port49_led2:yellow:status",
          "transceiverId": 49
        },
        "99": {
          "id": 99,
          "bluePath": "/sys/class/leds/port50_led1:blue:status",
          "yellowPath": "/sys/class/leds/port50_led1:yellow:status",
          "transceiverId": 50
        },
        "100": {
          "id": 100,
          "bluePath": "/sys/class/leds/port50_led2:blue:status",
          "yellowPath": "/sys/class/leds/port50_led2:yellow:status",
          "transceiverId": 50
        },
        "101": {
          "id": 101,
          "bluePath": "/sys/class/leds/port51_led1:blue:status",
          "yellowPath": "/sys/class/leds/port51_led1:yellow:status",
          "transceiverId": 51
        },
        "102": {
          "id": 102,
          "bluePath": "/sys/class/leds/port51_led2:blue:status",
          "yellowPath": "/sys/class/leds/port51_led2:yellow:status",
          "transceiverId": 51
        },
        "103": {
          "id": 103,
          "bluePath": "/sys/class/leds/port52_led1:blue:status",
          "yellowPath": "/sys/class/leds/port52_led1:yellow:status",
          "transceiverId": 52
        },
        "104": {
          "id": 104,
          "bluePath": "/sys/class/leds/port52_led2:blue:status",
          "yellowPath": "/sys/class/leds/port52_led2:yellow:status",
          "transceiverId": 52
        },
        "105": {
          "id": 105,
          "bluePath": "/sys/class/leds/port53_led1:blue:status",
          "yellowPath": "/sys/class/leds/port53_led1:yellow:status",
          "transceiverId": 53
        },
        "106": {
          "id": 106,
          "bluePath": "/sys/class/leds/port53_led2:blue:status",
          "yellowPath": "/sys/class/leds/port53_led2:yellow:status",
          "transceiverId": 53
        },
        "107": {
          "id": 107,
          "bluePath": "/sys/class/leds/port54_led1:blue:status",
          "yellowPath": "/sys/class/leds/port54_led1:yellow:status",
          "transceiverId": 54
        },
        "108": {
          "id": 108,
          "bluePath": "/sys/class/leds/port54_led2:blue:status",
          "yellowPath": "/sys/class/leds/port54_led2:yellow:status",
          "transceiverId": 54
        },
        "109": {
          "id": 109,
          "bluePath": "/sys/class/leds/port55_led1:blue:status",
          "yellowPath": "/sys/class/leds/port55_led1:yellow:status",
          "transceiverId": 55
        },
        "110": {
          "id": 110,
          "bluePath": "/sys/class/leds/port55_led2:blue:status",
          "yellowPath": "/sys/class/leds/port55_led2:yellow:status",
          "transceiverId": 55
        },
        "111": {
          "id": 111,
          "bluePath": "/sys/class/leds/port56_led1:blue:status",
          "yellowPath": "/sys/class/leds/port56_led1:yellow:status",
          "transceiverId": 56
        },
        "112": {
          "id": 112,
          "bluePath": "/sys/class/leds/port56_led2:blue:status",
          "yellowPath": "/sys/class/leds/port56_led2:yellow:status",
          "transceiverId": 56
        },
        "113": {
          "id": 113,
          "bluePath": "/sys/class/leds/port57_led1:blue:status",
          "yellowPath": "/sys/class/leds/port57_led1:yellow:status",
          "transceiverId": 57
        },
        "114": {
          "id": 114,
          "bluePath": "/sys/class/leds/port57_led2:blue:status",
          "yellowPath": "/sys/class/leds/port57_led2:yellow:status",
          "transceiverId": 57
        },
        "115": {
          "id": 115,
          "bluePath": "/sys/class/leds/port58_led1:blue:status",
          "yellowPath": "/sys/class/leds/port58_led1:yellow:status",
          "transceiverId": 58
        },
        "116": {
          "id": 116,
          "bluePath": "/sys/class/leds/port58_led2:blue:status",
          "yellowPath": "/sys/class/leds/port58_led2:yellow:status",
          "transceiverId": 58
        },
        "117": {
          "id": 117,
          "bluePath": "/sys/class/leds/port59_led1:blue:status",
          "yellowPath": "/sys/class/leds/port59_led1:yellow:status",
          "transceiverId": 59
        },
        "118": {
          "id": 118,
          "bluePath": "/sys/class/leds/port59_led2:blue:status",
          "yellowPath": "/sys/class/leds/port59_led2:yellow:status",
          "transceiverId": 59
        },
        "119": {
          "id": 119,
          "bluePath": "/sys/class/leds/port60_led1:blue:status",
          "yellowPath": "/sys/class/leds/port60_led1:yellow:status",
          "transceiverId": 60
        },
        "120": {
          "id": 120,
          "bluePath": "/sys/class/leds/port60_led2:blue:status",
          "yellowPath": "/sys/class/leds/port60_led2:yellow:status",
          "transceiverId": 60
        },
        "121": {
          "id": 121,
          "bluePath": "/sys/class/leds/port61_led1:blue:status",
          "yellowPath": "/sys/class/leds/port61_led1:yellow:status",
          "transceiverId": 61
        },
        "122": {
          "id": 122,
          "bluePath": "/sys/class/leds/port61_led2:blue:status",
          "yellowPath": "/sys/class/leds/port61_led2:yellow:status",
          "transceiverId": 61
        },
        "123": {
          "id": 123,
          "bluePath": "/sys/class/leds/port62_led1:blue:status",
          "yellowPath": "/sys/class/leds/port62_led1:yellow:status",
          "transceiverId": 62
        },
        "124": {
          "id": 124,
          "bluePath": "/sys/class/leds/port62_led2:blue:status",
          "yellowPath": "/sys/class/leds/port62_led2:yellow:status",
          "transceiverId": 62
        },
        "125": {
          "id": 125,
          "bluePath": "/sys/class/leds/port63_led1:blue:status",
          "yellowPath": "/sys/class/leds/port63_led1:yellow:status",
          "transceiverId": 63
        },
        "126": {
          "id": 126,
          "bluePath": "/sys/class/leds/port63_led2:blue:status",
          "yellowPath": "/sys/class/leds/port63_led2:yellow:status",
          "transceiverId": 63
        },
        "127": {
          "id": 127,
          "bluePath": "/sys/class/leds/port64_led1:blue:status",
          "yellowPath": "/sys/class/leds/port64_led1:yellow:status",
          "transceiverId": 64
        },
        "128": {
          "id": 128,
          "bluePath": "/sys/class/leds/port64_led2:blue:status",
          "yellowPath": "/sys/class/leds/port64_led2:yellow:status",
          "transceiverId": 64
        },
        "129": {
          "id": 129,
          "bluePath": "/sys/class/leds/port65_led1:blue:status",
          "yellowPath": "/sys/class/leds/port65_led1:yellow:status",
          "transceiverId": 65
        },
        "130": {
          "id": 130,
          "bluePath": "/sys/class/leds/port65_led2:blue:status",
          "yellowPath": "/sys/class/leds/port65_led2:yellow:status",
          "transceiverId": 65
        },
        "131": {
          "id": 131,
          "bluePath": "/sys/class/leds/port66_led1:blue:status",
          "yellowPath": "/sys/class/leds/port66_led1:yellow:status",
          "transceiverId": 66
        },
        "132": {
          "id": 132,
          "bluePath": "/sys/class/leds/port66_led2:blue:status",
          "yellowPath": "/sys/class/leds/port66_led2:yellow:status",
          "transceiverId": 66
        },
        "133": {
          "id": 133,
          "bluePath": "/sys/class/leds/port67_led1:blue:status",
          "yellowPath": "/sys/class/leds/port67_led1:yellow:status",
          "transceiverId": 67
        },
        "134": {
          "id": 134,
          "bluePath": "/sys/class/leds/port67_led2:blue:status",
          "yellowPath": "/sys/class/leds/port67_led2:yellow:status",
          "transceiverId": 67
        },
        "135": {
          "id": 135,
          "bluePath": "/sys/class/leds/port68_led1:blue:status",
          "yellowPath": "/sys/class/leds/port68_led1:yellow:status",
          "transceiverId": 68
        },
        "136": {
          "id": 136,
          "bluePath": "/sys/class/leds/port68_led2:blue:status",
          "yellowPath": "/sys/class/leds/port68_led2:yellow:status",
          "transceiverId": 68
        },
        "137": {
          "id": 137,
          "bluePath": "/sys/class/leds/port69_led1:blue:status",
          "yellowPath": "/sys/class/leds/port69_led1:yellow:status",
          "transceiverId": 69
        },
        "138": {
          "id": 138,
          "bluePath": "/sys/class/leds/port69_led2:blue:status",
          "yellowPath": "/sys/class/leds/port69_led2:yellow:status",
          "transceiverId": 69
        },
        "139": {
          "id": 139,
          "bluePath": "/sys/class/leds/port70_led1:blue:status",
          "yellowPath": "/sys/class/leds/port70_led1:yellow:status",
          "transceiverId": 70
        },
        "140": {
          "id": 140,
          "bluePath": "/sys/class/leds/port70_led2:blue:status",
          "yellowPath": "/sys/class/leds/port70_led2:yellow:status",
          "transceiverId": 70
        },
        "141": {
          "id": 141,
          "bluePath": "/sys/class/leds/port71_led1:blue:status",
          "yellowPath": "/sys/class/leds/port71_led1:yellow:status",
          "transceiverId": 71
        },
        "142": {
          "id": 142,
          "bluePath": "/sys/class/leds/port71_led2:blue:status",
          "yellowPath": "/sys/class/leds/port71_led2:yellow:status",
          "transceiverId": 71
        },
        "143": {
          "id": 143,
          "bluePath": "/sys/class/leds/port72_led1:blue:status",
          "yellowPath": "/sys/class/leds/port72_led1:yellow:status",
          "transceiverId": 72
        },
        "144": {
          "id": 144,
          "bluePath": "/sys/class/leds/port72_led2:blue:status",
          "yellowPath": "/sys/class/leds/port72_led2:yellow:status",
          "transceiverId": 72
        },
        "145": {
          "id": 145,
          "bluePath": "/sys/class/leds/port73_led1:blue:status",
          "yellowPath": "/sys/class/leds/port73_led1:yellow:status",
          "transceiverId": 73
        },
        "146": {
          "id": 146,
          "bluePath": "/sys/class/leds/port73_led2:blue:status",
          "yellowPath": "/sys/class/leds/port73_led2:yellow:status",
          "transceiverId": 73
        },
        "147": {
          "id": 147,
          "bluePath": "/sys/class/leds/port74_led1:blue:status",
          "yellowPath": "/sys/class/leds/port74_led1:yellow:status",
          "transceiverId": 74
        },
        "148": {
          "id": 148,
          "bluePath": "/sys/class/leds/port74_led2:blue:status",
          "yellowPath": "/sys/class/leds/port74_led2:yellow:status",
          "transceiverId": 74
        },
        "149": {
          "id": 149,
          "bluePath": "/sys/class/leds/port75_led1:blue:status",
          "yellowPath": "/sys/class/leds/port75_led1:yellow:status",
          "transceiverId": 75
        },
        "150": {
          "id": 150,
          "bluePath": "/sys/class/leds/port75_led2:blue:status",
          "yellowPath": "/sys/class/leds/port75_led2:yellow:status",
          "transceiverId": 75
        },
        "151": {
          "id": 151,
          "bluePath": "/sys/class/leds/port76_led1:blue:status",
          "yellowPath": "/sys/class/leds/port76_led1:yellow:status",
          "transceiverId": 76
        },
        "152": {
          "id": 152,
          "bluePath": "/sys/class/leds/port76_led2:blue:status",
          "yellowPath": "/sys/class/leds/port76_led2:yellow:status",
          "transceiverId": 76
        },
        "153": {
          "id": 153,
          "bluePath": "/sys/class/leds/port77_led1:blue:status",
          "yellowPath": "/sys/class/leds/port77_led1:yellow:status",
          "transceiverId": 77
        },
        "154": {
          "id": 154,
          "bluePath": "/sys/class/leds/port77_led2:blue:status",
          "yellowPath": "/sys/class/leds/port77_led2:yellow:status",
          "transceiverId": 77
        },
        "155": {
          "id": 155,
          "bluePath": "/sys/class/leds/port78_led1:blue:status",
          "yellowPath": "/sys/class/leds/port78_led1:yellow:status",
          "transceiverId": 78
        },
        "156": {
          "id": 156,
          "bluePath": "/sys/class/leds/port78_led2:blue:status",
          "yellowPath": "/sys/class/leds/port78_led2:yellow:status",
          "transceiverId": 78
        },
        "157": {
          "id": 157,
          "bluePath": "/sys/class/leds/port79_led1:blue:status",
          "yellowPath": "/sys/class/leds/port79_led1:yellow:status",
          "transceiverId": 79
        },
        "158": {
          "id": 158,
          "bluePath": "/sys/class/leds/port79_led2:blue:status",
          "yellowPath": "/sys/class/leds/port79_led2:yellow:status",
          "transceiverId": 79
        },
        "159": {
          "id": 159,
          "bluePath": "/sys/class/leds/port80_led1:blue:status",
          "yellowPath": "/sys/class/leds/port80_led1:yellow:status",
          "transceiverId": 80
        },
        "160": {
          "id": 160,
          "bluePath": "/sys/class/leds/port80_led2:blue:status",
          "yellowPath": "/sys/class/leds/port80_led2:yellow:status",
          "transceiverId": 80
        },
        "161": {
          "id": 161,
          "bluePath": "/sys/class/leds/port81_led1:blue:status",
          "yellowPath": "/sys/class/leds/port81_led1:yellow:status",
          "transceiverId": 81
        },
        "162": {
          "id": 162,
          "bluePath": "/sys/class/leds/port81_led2:blue:status",
          "yellowPath": "/sys/class/leds/port81_led2:yellow:status",
          "transceiverId": 81
        },
        "163": {
          "id": 163,
          "bluePath": "/sys/class/leds/port82_led1:blue:status",
          "yellowPath": "/sys/class/leds/port82_led1:yellow:status",
          "transceiverId": 82
        },
        "164": {
          "id": 164,
          "bluePath": "/sys/class/leds/port82_led2:blue:status",
          "yellowPath": "/sys/class/leds/port82_led2:yellow:status",
          "transceiverId": 82
        },
        "165": {
          "id": 165,
          "bluePath": "/sys/class/leds/port83_led1:blue:status",
          "yellowPath": "/sys/class/leds/port83_led1:yellow:status",
          "transceiverId": 83
        },
        "166": {
          "id": 166,
          "bluePath": "/sys/class/leds/port83_led2:blue:status",
          "yellowPath": "/sys/class/leds/port83_led2:yellow:status",
          "transceiverId": 83
        },
        "167": {
          "id": 167,
          "bluePath": "/sys/class/leds/port84_led1:blue:status",
          "yellowPath": "/sys/class/leds/port84_led1:yellow:status",
          "transceiverId": 84
        },
        "168": {
          "id": 168,
          "bluePath": "/sys/class/leds/port84_led2:blue:status",
          "yellowPath": "/sys/class/leds/port84_led2:yellow:status",
          "transceiverId": 84
        },
        "169": {
          "id": 169,
          "bluePath": "/sys/class/leds/port85_led1:blue:status",
          "yellowPath": "/sys/class/leds/port85_led1:yellow:status",
          "transceiverId": 85
        },
        "170": {
          "id": 170,
          "bluePath": "/sys/class/leds/port85_led2:blue:status",
          "yellowPath": "/sys/class/leds/port85_led2:yellow:status",
          "transceiverId": 85
        },
        "171": {
          "id": 171,
          "bluePath": "/sys/class/leds/port86_led1:blue:status",
          "yellowPath": "/sys/class/leds/port86_led1:yellow:status",
          "transceiverId": 86
        },
        "172": {
          "id": 172,
          "bluePath": "/sys/class/leds/port86_led2:blue:status",
          "yellowPath": "/sys/class/leds/port86_led2:yellow:status",
          "transceiverId": 86
        },
        "173": {
          "id": 173,
          "bluePath": "/sys/class/leds/port87_led1:blue:status",
          "yellowPath": "/sys/class/leds/port87_led1:yellow:status",
          "transceiverId": 87
        },
        "174": {
          "id": 174,
          "bluePath": "/sys/class/leds/port87_led2:blue:status",
          "yellowPath": "/sys/class/leds/port87_led2:yellow:status",
          "transceiverId": 87
        },
        "175": {
          "id": 175,
          "bluePath": "/sys/class/leds/port88_led1:blue:status",
          "yellowPath": "/sys/class/leds/port88_led1:yellow:status",
          "transceiverId": 88
        },
        "176": {
          "id": 176,
          "bluePath": "/sys/class/leds/port88_led2:blue:status",
          "yellowPath": "/sys/class/leds/port88_led2:yellow:status",
          "transceiverId": 88
        },
        "177": {
          "id": 177,
          "bluePath": "/sys/class/leds/port89_led1:blue:status",
          "yellowPath": "/sys/class/leds/port89_led1:yellow:status",
          "transceiverId": 89
        },
        "178": {
          "id": 178,
          "bluePath": "/sys/class/leds/port89_led2:blue:status",
          "yellowPath": "/sys/class/leds/port89_led2:yellow:status",
          "transceiverId": 89
        },
        "179": {
          "id": 179,
          "bluePath": "/sys/class/leds/port90_led1:blue:status",
          "yellowPath": "/sys/class/leds/port90_led1:yellow:status",
          "transceiverId": 90
        },
        "180": {
          "id": 180,
          "bluePath": "/sys/class/leds/port90_led2:blue:status",
          "yellowPath": "/sys/class/leds/port90_led2:yellow:status",
          "transceiverId": 90
        },
        "181": {
          "id": 181,
          "bluePath": "/sys/class/leds/port91_led1:blue:status",
          "yellowPath": "/sys/class/leds/port91_led1:yellow:status",
          "transceiverId": 91
        },
        "182": {
          "id": 182,
          "bluePath": "/sys/class/leds/port91_led2:blue:status",
          "yellowPath": "/sys/class/leds/port91_led2:yellow:status",
          "transceiverId": 91
        },
        "183": {
          "id": 183,
          "bluePath": "/sys/class/leds/port92_led1:blue:status",
          "yellowPath": "/sys/class/leds/port92_led1:yellow:status",
          "transceiverId": 92
        },
        "184": {
          "id": 184,
          "bluePath": "/sys/class/leds/port92_led2:blue:status",
          "yellowPath": "/sys/class/leds/port92_led2:yellow:status",
          "transceiverId": 92
        },
        "185": {
          "id": 185,
          "bluePath": "/sys/class/leds/port93_led1:blue:status",
          "yellowPath": "/sys/class/leds/port93_led1:yellow:status",
          "transceiverId": 93
        },
        "186": {
          "id": 186,
          "bluePath": "/sys/class/leds/port93_led2:blue:status",
          "yellowPath": "/sys/class/leds/port93_led2:yellow:status",
          "transceiverId": 93
        },
        "187": {
          "id": 187,
          "bluePath": "/sys/class/leds/port94_led1:blue:status",
          "yellowPath": "/sys/class/leds/port94_led1:yellow:status",
          "transceiverId": 94
        },
        "188": {
          "id": 188,
          "bluePath": "/sys/class/leds/port94_led2:blue:status",
          "yellowPath": "/sys/class/leds/port94_led2:yellow:status",
          "transceiverId": 94
        },
        "189": {
          "id": 189,
          "bluePath": "/sys/class/leds/port95_led1:blue:status",
          "yellowPath": "/sys/class/leds/port95_led1:yellow:status",
          "transceiverId": 95
        },
        "190": {
          "id": 190,
          "bluePath": "/sys/class/leds/port95_led2:blue:status",
          "yellowPath": "/sys/class/leds/port95_led2:yellow:status",
          "transceiverId": 95
        },
        "191": {
          "id": 191,
          "bluePath": "/sys/class/leds/port96_led1:blue:status",
          "yellowPath": "/sys/class/leds/port96_led1:yellow:status",
          "transceiverId": 96
        },
        "192": {
          "id": 192,
          "bluePath": "/sys/class/leds/port96_led2:blue:status",
          "yellowPath": "/sys/class/leds/port96_led2:yellow:status",
          "transceiverId": 96
        },
        "193": {
          "id": 193,
          "bluePath": "/sys/class/leds/port97_led1:blue:status",
          "yellowPath": "/sys/class/leds/port97_led1:yellow:status",
          "transceiverId": 97
        },
        "194": {
          "id": 194,
          "bluePath": "/sys/class/leds/port97_led2:blue:status",
          "yellowPath": "/sys/class/leds/port97_led2:yellow:status",
          "transceiverId": 97
        },
        "195": {
          "id": 195,
          "bluePath": "/sys/class/leds/port98_led1:blue:status",
          "yellowPath": "/sys/class/leds/port98_led1:yellow:status",
          "transceiverId": 98
        },
        "196": {
          "id": 196,
          "bluePath": "/sys/class/leds/port98_led2:blue:status",
          "yellowPath": "/sys/class/leds/port98_led2:yellow:status",
          "transceiverId": 98
        },
        "197": {
          "id": 197,
          "bluePath": "/sys/class/leds/port99_led1:blue:status",
          "yellowPath": "/sys/class/leds/port99_led1:yellow:status",
          "transceiverId": 99
        },
        "198": {
          "id": 198,
          "bluePath": "/sys/class/leds/port99_led2:blue:status",
          "yellowPath": "/sys/class/leds/port99_led2:yellow:status",
          "transceiverId": 99
        },
        "199": {
          "id": 199,
          "bluePath": "/sys/class/leds/port100_led1:blue:status",
          "yellowPath": "/sys/class/leds/port100_led1:yellow:status",
          "transceiverId": 100
        },
        "200": {
          "id": 200,
          "bluePath": "/sys/class/leds/port100_led2:blue:status",
          "yellowPath": "/sys/class/leds/port100_led2:yellow:status",
          "transceiverId": 100
        },
        "201": {
          "id": 201,
          "bluePath": "/sys/class/leds/port101_led1:blue:status",
          "yellowPath": "/sys/class/leds/port101_led1:yellow:status",
          "transceiverId": 101
        },
        "202": {
          "id": 202,
          "bluePath": "/sys/class/leds/port101_led2:blue:status",
          "yellowPath": "/sys/class/leds/port101_led2:yellow:status",
          "transceiverId": 101
        },
        "203": {
          "id": 203,
          "bluePath": "/sys/class/leds/port102_led1:blue:status",
          "yellowPath": "/sys/class/leds/port102_led1:yellow:status",
          "transceiverId": 102
        },
        "204": {
          "id": 204,
          "bluePath": "/sys/class/leds/port102_led2:blue:status",
          "yellowPath": "/sys/class/leds/port102_led2:yellow:status",
          "transceiverId": 102
        },
        "205": {
          "id": 205,
          "bluePath": "/sys/class/leds/port103_led1:blue:status",
          "yellowPath": "/sys/class/leds/port103_led1:yellow:status",
          "transceiverId": 103
        },
        "206": {
          "id": 206,
          "bluePath": "/sys/class/leds/port103_led2:blue:status",
          "yellowPath": "/sys/class/leds/port103_led2:yellow:status",
          "transceiverId": 103
        },
        "207": {
          "id": 207,
          "bluePath": "/sys/class/leds/port104_led1:blue:status",
          "yellowPath": "/sys/class/leds/port104_led1:yellow:status",
          "transceiverId": 104
        },
        "208": {
          "id": 208,
          "bluePath": "/sys/class/leds/port104_led2:blue:status",
          "yellowPath": "/sys/class/leds/port104_led2:yellow:status",
          "transceiverId": 104
        },
        "209": {
          "id": 209,
          "bluePath": "/sys/class/leds/port105_led1:blue:status",
          "yellowPath": "/sys/class/leds/port105_led1:yellow:status",
          "transceiverId": 105
        },
        "210": {
          "id": 210,
          "bluePath": "/sys/class/leds/port105_led2:blue:status",
          "yellowPath": "/sys/class/leds/port105_led2:yellow:status",
          "transceiverId": 105
        },
        "211": {
          "id": 211,
          "bluePath": "/sys/class/leds/port106_led1:blue:status",
          "yellowPath": "/sys/class/leds/port106_led1:yellow:status",
          "transceiverId": 106
        },
        "212": {
          "id": 212,
          "bluePath": "/sys/class/leds/port106_led2:blue:status",
          "yellowPath": "/sys/class/leds/port106_led2:yellow:status",
          "transceiverId": 106
        },
        "213": {
          "id": 213,
          "bluePath": "/sys/class/leds/port107_led1:blue:status",
          "yellowPath": "/sys/class/leds/port107_led1:yellow:status",
          "transceiverId": 107
        },
        "214": {
          "id": 214,
          "bluePath": "/sys/class/leds/port107_led2:blue:status",
          "yellowPath": "/sys/class/leds/port107_led2:yellow:status",
          "transceiverId": 107
        },
        "215": {
          "id": 215,
          "bluePath": "/sys/class/leds/port108_led1:blue:status",
          "yellowPath": "/sys/class/leds/port108_led1:yellow:status",
          "transceiverId": 108
        },
        "216": {
          "id": 216,
          "bluePath": "/sys/class/leds/port108_led2:blue:status",
          "yellowPath": "/sys/class/leds/port108_led2:yellow:status",
          "transceiverId": 108
        },
        "217": {
          "id": 217,
          "bluePath": "/sys/class/leds/port109_led1:blue:status",
          "yellowPath": "/sys/class/leds/port109_led1:yellow:status",
          "transceiverId": 109
        },
        "218": {
          "id": 218,
          "bluePath": "/sys/class/leds/port109_led2:blue:status",
          "yellowPath": "/sys/class/leds/port109_led2:yellow:status",
          "transceiverId": 109
        },
        "219": {
          "id": 219,
          "bluePath": "/sys/class/leds/port110_led1:blue:status",
          "yellowPath": "/sys/class/leds/port110_led1:yellow:status",
          "transceiverId": 110
        },
        "220": {
          "id": 220,
          "bluePath": "/sys/class/leds/port110_led2:blue:status",
          "yellowPath": "/sys/class/leds/port110_led2:yellow:status",
          "transceiverId": 110
        },
        "221": {
          "id": 221,
          "bluePath": "/sys/class/leds/port111_led1:blue:status",
          "yellowPath": "/sys/class/leds/port111_led1:yellow:status",
          "transceiverId": 111
        },
        "222": {
          "id": 222,
          "bluePath": "/sys/class/leds/port111_led2:blue:status",
          "yellowPath": "/sys/class/leds/port111_led2:yellow:status",
          "transceiverId": 111
        },
        "223": {
          "id": 223,
          "bluePath": "/sys/class/leds/port112_led1:blue:status",
          "yellowPath": "/sys/class/leds/port112_led1:yellow:status",
          "transceiverId": 112
        },
        "224": {
          "id": 224,
          "bluePath": "/sys/class/leds/port112_led2:blue:status",
          "yellowPath": "/sys/class/leds/port112_led2:yellow:status",
          "transceiverId": 112
        },
        "225": {
          "id": 225,
          "bluePath": "/sys/class/leds/port113_led1:blue:status",
          "yellowPath": "/sys/class/leds/port113_led1:yellow:status",
          "transceiverId": 113
        },
        "226": {
          "id": 226,
          "bluePath": "/sys/class/leds/port113_led2:blue:status",
          "yellowPath": "/sys/class/leds/port113_led2:yellow:status",
          "transceiverId": 113
        },
        "227": {
          "id": 227,
          "bluePath": "/sys/class/leds/port114_led1:blue:status",
          "yellowPath": "/sys/class/leds/port114_led1:yellow:status",
          "transceiverId": 114
        },
        "228": {
          "id": 228,
          "bluePath": "/sys/class/leds/port114_led2:blue:status",
          "yellowPath": "/sys/class/leds/port114_led2:yellow:status",
          "transceiverId": 114
        },
        "229": {
          "id": 229,
          "bluePath": "/sys/class/leds/port115_led1:blue:status",
          "yellowPath": "/sys/class/leds/port115_led1:yellow:status",
          "transceiverId": 115
        },
        "230": {
          "id": 230,
          "bluePath": "/sys/class/leds/port115_led2:blue:status",
          "yellowPath": "/sys/class/leds/port115_led2:yellow:status",
          "transceiverId": 115
        },
        "231": {
          "id": 231,
          "bluePath": "/sys/class/leds/port116_led1:blue:status",
          "yellowPath": "/sys/class/leds/port116_led1:yellow:status",
          "transceiverId": 116
        },
        "232": {
          "id": 232,
          "bluePath": "/sys/class/leds/port116_led2:blue:status",
          "yellowPath": "/sys/class/leds/port116_led2:yellow:status",
          "transceiverId": 116
        },
        "233": {
          "id": 233,
          "bluePath": "/sys/class/leds/port117_led1:blue:status",
          "yellowPath": "/sys/class/leds/port117_led1:yellow:status",
          "transceiverId": 117
        },
        "234": {
          "id": 234,
          "bluePath": "/sys/class/leds/port117_led2:blue:status",
          "yellowPath": "/sys/class/leds/port117_led2:yellow:status",
          "transceiverId": 117
        },
        "235": {
          "id": 235,
          "bluePath": "/sys/class/leds/port118_led1:blue:status",
          "yellowPath": "/sys/class/leds/port118_led1:yellow:status",
          "transceiverId": 118
        },
        "236": {
          "id": 236,
          "bluePath": "/sys/class/leds/port118_led2:blue:status",
          "yellowPath": "/sys/class/leds/port118_led2:yellow:status",
          "transceiverId": 118
        },
        "237": {
          "id": 237,
          "bluePath": "/sys/class/leds/port119_led1:blue:status",
          "yellowPath": "/sys/class/leds/port119_led1:yellow:status",
          "transceiverId": 119
        },
        "238": {
          "id": 238,
          "bluePath": "/sys/class/leds/port119_led2:blue:status",
          "yellowPath": "/sys/class/leds/port119_led2:yellow:status",
          "transceiverId": 119
        },
        "239": {
          "id": 239,
          "bluePath": "/sys/class/leds/port120_led1:blue:status",
          "yellowPath": "/sys/class/leds/port120_led1:yellow:status",
          "transceiverId": 120
        },
        "240": {
          "id": 240,
          "bluePath": "/sys/class/leds/port120_led2:blue:status",
          "yellowPath": "/sys/class/leds/port120_led2:yellow:status",
          "transceiverId": 120
        },
        "241": {
          "id": 241,
          "bluePath": "/sys/class/leds/port121_led1:blue:status",
          "yellowPath": "/sys/class/leds/port121_led1:yellow:status",
          "transceiverId": 121
        },
        "242": {
          "id": 242,
          "bluePath": "/sys/class/leds/port121_led2:blue:status",
          "yellowPath": "/sys/class/leds/port121_led2:yellow:status",
          "transceiverId": 121
        },
        "243": {
          "id": 243,
          "bluePath": "/sys/class/leds/port122_led1:blue:status",
          "yellowPath": "/sys/class/leds/port122_led1:yellow:status",
          "transceiverId": 122
        },
        "244": {
          "id": 244,
          "bluePath": "/sys/class/leds/port122_led2:blue:status",
          "yellowPath": "/sys/class/leds/port122_led2:yellow:status",
          "transceiverId": 122
        },
        "245": {
          "id": 245,
          "bluePath": "/sys/class/leds/port123_led1:blue:status",
          "yellowPath": "/sys/class/leds/port123_led1:yellow:status",
          "transceiverId": 123
        },
        "246": {
          "id": 246,
          "bluePath": "/sys/class/leds/port123_led2:blue:status",
          "yellowPath": "/sys/class/leds/port123_led2:yellow:status",
          "transceiverId": 123
        },
        "247": {
          "id": 247,
          "bluePath": "/sys/class/leds/port124_led1:blue:status",
          "yellowPath": "/sys/class/leds/port124_led1:yellow:status",
          "transceiverId": 124
        },
        "248": {
          "id": 248,
          "bluePath": "/sys/class/leds/port124_led2:blue:status",
          "yellowPath": "/sys/class/leds/port124_led2:yellow:status",
          "transceiverId": 124
        },
        "249": {
          "id": 249,
          "bluePath": "/sys/class/leds/port125_led1:blue:status",
          "yellowPath": "/sys/class/leds/port125_led1:yellow:status",
          "transceiverId": 125
        },
        "250": {
          "id": 250,
          "bluePath": "/sys/class/leds/port125_led2:blue:status",
          "yellowPath": "/sys/class/leds/port125_led2:yellow:status",
          "transceiverId": 125
        },
        "251": {
          "id": 251,
          "bluePath": "/sys/class/leds/port126_led1:blue:status",
          "yellowPath": "/sys/class/leds/port126_led1:yellow:status",
          "transceiverId": 126
        },
        "252": {
          "id": 252,
          "bluePath": "/sys/class/leds/port126_led2:blue:status",
          "yellowPath": "/sys/class/leds/port126_led2:yellow:status",
          "transceiverId": 126
        },
        "253": {
          "id": 253,
          "bluePath": "/sys/class/leds/port127_led1:blue:status",
          "yellowPath": "/sys/class/leds/port127_led1:yellow:status",
          "transceiverId": 127
        },
        "254": {
          "id": 254,
          "bluePath": "/sys/class/leds/port127_led2:blue:status",
          "yellowPath": "/sys/class/leds/port127_led2:yellow:status",
          "transceiverId": 127
        },
        "255": {
          "id": 255,
          "bluePath": "/sys/class/leds/port128_led1:blue:status",
          "yellowPath": "/sys/class/leds/port128_led1:yellow:status",
          "transceiverId": 128
        },
        "256": {
          "id": 256,
          "bluePath": "/sys/class/leds/port128_led2:blue:status",
          "yellowPath": "/sys/class/leds/port128_led2:yellow:status",
          "transceiverId": 128
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildMeru800bfaPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Meru800bfaBspPlatformMapping::Meru800bfaBspPlatformMapping()
    : BspPlatformMapping(
          buildMeru800bfaPlatformMapping(kJsonBspPlatformMappingStr)) {}

Meru800bfaBspPlatformMapping::Meru800bfaBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildMeru800bfaPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
