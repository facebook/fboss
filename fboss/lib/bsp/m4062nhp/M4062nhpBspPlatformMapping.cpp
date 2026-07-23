/*
 *  Copyright (c) 2026 Nexthop Systems Inc.
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "fboss/lib/bsp/m4062nhp/M4062nhpBspPlatformMapping.h"

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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_reset_1",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_present_1",
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
            "5": 1,
            "6": 1,
            "7": 1,
            "8": 1
          }
        },
        "2": {
          "tcvrId": 2,
          "accessControl": {
            "controllerId": "2",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_2/xcvr_reset_2",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_2/xcvr_present_2",
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
            "1": 2,
            "2": 2,
            "3": 2,
            "4": 2,
            "5": 2,
            "6": 2,
            "7": 2,
            "8": 2
          }
        },
        "3": {
          "tcvrId": 3,
          "accessControl": {
            "controllerId": "3",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_33/xcvr_reset_33",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_33/xcvr_present_33",
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
            "1": 3,
            "2": 3,
            "3": 3,
            "4": 3,
            "5": 3,
            "6": 3,
            "7": 3,
            "8": 3
          }
        },
        "4": {
          "tcvrId": 4,
          "accessControl": {
            "controllerId": "4",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_34/xcvr_reset_34",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_34/xcvr_present_34",
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
            "1": 4,
            "2": 4,
            "3": 4,
            "4": 4,
            "5": 4,
            "6": 4,
            "7": 4,
            "8": 4
          }
        },
        "5": {
          "tcvrId": 5,
          "accessControl": {
            "controllerId": "5",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_35/xcvr_reset_35",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_35/xcvr_present_35",
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
            "1": 5,
            "2": 5,
            "3": 5,
            "4": 5,
            "5": 5,
            "6": 5,
            "7": 5,
            "8": 5
          }
        },
        "6": {
          "tcvrId": 6,
          "accessControl": {
            "controllerId": "6",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_36/xcvr_reset_36",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_36/xcvr_present_36",
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
            "1": 6,
            "2": 6,
            "3": 6,
            "4": 6,
            "5": 6,
            "6": 6,
            "7": 6,
            "8": 6
          }
        },
        "7": {
          "tcvrId": 7,
          "accessControl": {
            "controllerId": "7",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_97/xcvr_reset_97",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_97/xcvr_present_97",
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
            "1": 7,
            "2": 7,
            "3": 7,
            "4": 7,
            "5": 7,
            "6": 7,
            "7": 7,
            "8": 7
          }
        },
        "8": {
          "tcvrId": 8,
          "accessControl": {
            "controllerId": "8",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_98/xcvr_reset_98",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_98/xcvr_present_98",
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
            "1": 8,
            "2": 8,
            "3": 8,
            "4": 8,
            "5": 8,
            "6": 8,
            "7": 8,
            "8": 8
          }
        },
        "9": {
          "tcvrId": 9,
          "accessControl": {
            "controllerId": "9",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr_reset_3",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr_present_3",
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
            "1": 9,
            "2": 9,
            "3": 9,
            "4": 9,
            "5": 9,
            "6": 9,
            "7": 9,
            "8": 9
          }
        },
        "10": {
          "tcvrId": 10,
          "accessControl": {
            "controllerId": "10",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_4/xcvr_reset_4",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_4/xcvr_present_4",
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
            "1": 10,
            "2": 10,
            "3": 10,
            "4": 10,
            "5": 10,
            "6": 10,
            "7": 10,
            "8": 10
          }
        },
        "11": {
          "tcvrId": 11,
          "accessControl": {
            "controllerId": "11",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_37/xcvr_reset_37",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_37/xcvr_present_37",
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
            "1": 11,
            "2": 11,
            "3": 11,
            "4": 11,
            "5": 11,
            "6": 11,
            "7": 11,
            "8": 11
          }
        },
        "12": {
          "tcvrId": 12,
          "accessControl": {
            "controllerId": "12",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_38/xcvr_reset_38",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_38/xcvr_present_38",
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
            "1": 12,
            "2": 12,
            "3": 12,
            "4": 12,
            "5": 12,
            "6": 12,
            "7": 12,
            "8": 12
          }
        },
        "13": {
          "tcvrId": 13,
          "accessControl": {
            "controllerId": "13",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_39/xcvr_reset_39",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_39/xcvr_present_39",
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
            "1": 13,
            "2": 13,
            "3": 13,
            "4": 13,
            "5": 13,
            "6": 13,
            "7": 13,
            "8": 13
          }
        },
        "14": {
          "tcvrId": 14,
          "accessControl": {
            "controllerId": "14",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_40/xcvr_reset_40",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_40/xcvr_present_40",
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
            "1": 14,
            "2": 14,
            "3": 14,
            "4": 14,
            "5": 14,
            "6": 14,
            "7": 14,
            "8": 14
          }
        },
        "15": {
          "tcvrId": 15,
          "accessControl": {
            "controllerId": "15",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_99/xcvr_reset_99",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_99/xcvr_present_99",
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
            "1": 15,
            "2": 15,
            "3": 15,
            "4": 15,
            "5": 15,
            "6": 15,
            "7": 15,
            "8": 15
          }
        },
        "16": {
          "tcvrId": 16,
          "accessControl": {
            "controllerId": "16",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_100/xcvr_reset_100",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_100/xcvr_present_100",
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
            "1": 16,
            "2": 16,
            "3": 16,
            "4": 16,
            "5": 16,
            "6": 16,
            "7": 16,
            "8": 16
          }
        },
        "17": {
          "tcvrId": 17,
          "accessControl": {
            "controllerId": "17",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_5/xcvr_reset_5",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_5/xcvr_present_5",
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
            "1": 17,
            "2": 17,
            "3": 17,
            "4": 17,
            "5": 17,
            "6": 17,
            "7": 17,
            "8": 17
          }
        },
        "18": {
          "tcvrId": 18,
          "accessControl": {
            "controllerId": "18",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_6/xcvr_reset_6",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_6/xcvr_present_6",
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
            "1": 18,
            "2": 18,
            "3": 18,
            "4": 18,
            "5": 18,
            "6": 18,
            "7": 18,
            "8": 18
          }
        },
        "19": {
          "tcvrId": 19,
          "accessControl": {
            "controllerId": "19",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_41/xcvr_reset_41",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_41/xcvr_present_41",
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
            "1": 19,
            "2": 19,
            "3": 19,
            "4": 19,
            "5": 19,
            "6": 19,
            "7": 19,
            "8": 19
          }
        },
        "20": {
          "tcvrId": 20,
          "accessControl": {
            "controllerId": "20",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_42/xcvr_reset_42",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_42/xcvr_present_42",
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
            "1": 20,
            "2": 20,
            "3": 20,
            "4": 20,
            "5": 20,
            "6": 20,
            "7": 20,
            "8": 20
          }
        },
        "21": {
          "tcvrId": 21,
          "accessControl": {
            "controllerId": "21",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_43/xcvr_reset_43",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_43/xcvr_present_43",
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
            "1": 21,
            "2": 21,
            "3": 21,
            "4": 21,
            "5": 21,
            "6": 21,
            "7": 21,
            "8": 21
          }
        },
        "22": {
          "tcvrId": 22,
          "accessControl": {
            "controllerId": "22",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_44/xcvr_reset_44",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_44/xcvr_present_44",
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
            "1": 22,
            "2": 22,
            "3": 22,
            "4": 22,
            "5": 22,
            "6": 22,
            "7": 22,
            "8": 22
          }
        },
        "23": {
          "tcvrId": 23,
          "accessControl": {
            "controllerId": "23",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_101/xcvr_reset_101",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_101/xcvr_present_101",
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
            "1": 23,
            "2": 23,
            "3": 23,
            "4": 23,
            "5": 23,
            "6": 23,
            "7": 23,
            "8": 23
          }
        },
        "24": {
          "tcvrId": 24,
          "accessControl": {
            "controllerId": "24",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_102/xcvr_reset_102",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_102/xcvr_present_102",
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
            "1": 24,
            "2": 24,
            "3": 24,
            "4": 24,
            "5": 24,
            "6": 24,
            "7": 24,
            "8": 24
          }
        },
        "25": {
          "tcvrId": 25,
          "accessControl": {
            "controllerId": "25",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_7/xcvr_reset_7",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_7/xcvr_present_7",
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
            "1": 25,
            "2": 25,
            "3": 25,
            "4": 25,
            "5": 25,
            "6": 25,
            "7": 25,
            "8": 25
          }
        },
        "26": {
          "tcvrId": 26,
          "accessControl": {
            "controllerId": "26",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_8/xcvr_reset_8",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_8/xcvr_present_8",
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
            "1": 26,
            "2": 26,
            "3": 26,
            "4": 26,
            "5": 26,
            "6": 26,
            "7": 26,
            "8": 26
          }
        },
        "27": {
          "tcvrId": 27,
          "accessControl": {
            "controllerId": "27",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_45/xcvr_reset_45",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_45/xcvr_present_45",
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
            "1": 27,
            "2": 27,
            "3": 27,
            "4": 27,
            "5": 27,
            "6": 27,
            "7": 27,
            "8": 27
          }
        },
        "28": {
          "tcvrId": 28,
          "accessControl": {
            "controllerId": "28",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_46/xcvr_reset_46",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_46/xcvr_present_46",
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
            "1": 28,
            "2": 28,
            "3": 28,
            "4": 28,
            "5": 28,
            "6": 28,
            "7": 28,
            "8": 28
          }
        },
        "29": {
          "tcvrId": 29,
          "accessControl": {
            "controllerId": "29",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_47/xcvr_reset_47",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_47/xcvr_present_47",
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
            "1": 29,
            "2": 29,
            "3": 29,
            "4": 29,
            "5": 29,
            "6": 29,
            "7": 29,
            "8": 29
          }
        },
        "30": {
          "tcvrId": 30,
          "accessControl": {
            "controllerId": "30",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_48/xcvr_reset_48",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_48/xcvr_present_48",
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
            "1": 30,
            "2": 30,
            "3": 30,
            "4": 30,
            "5": 30,
            "6": 30,
            "7": 30,
            "8": 30
          }
        },
        "31": {
          "tcvrId": 31,
          "accessControl": {
            "controllerId": "31",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_103/xcvr_reset_103",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_103/xcvr_present_103",
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
            "1": 31,
            "2": 31,
            "3": 31,
            "4": 31,
            "5": 31,
            "6": 31,
            "7": 31,
            "8": 31
          }
        },
        "32": {
          "tcvrId": 32,
          "accessControl": {
            "controllerId": "32",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_104/xcvr_reset_104",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_104/xcvr_present_104",
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
            "1": 32,
            "2": 32,
            "3": 32,
            "4": 32,
            "5": 32,
            "6": 32,
            "7": 32,
            "8": 32
          }
        },
        "33": {
          "tcvrId": 33,
          "accessControl": {
            "controllerId": "33",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_9/xcvr_reset_9",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_9/xcvr_present_9",
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
            "1": 33,
            "2": 33,
            "3": 33,
            "4": 33,
            "5": 33,
            "6": 33,
            "7": 33,
            "8": 33
          }
        },
        "34": {
          "tcvrId": 34,
          "accessControl": {
            "controllerId": "34",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_10/xcvr_reset_10",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_10/xcvr_present_10",
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
            "1": 34,
            "2": 34,
            "3": 34,
            "4": 34,
            "5": 34,
            "6": 34,
            "7": 34,
            "8": 34
          }
        },
        "35": {
          "tcvrId": 35,
          "accessControl": {
            "controllerId": "35",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_49/xcvr_reset_49",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_49/xcvr_present_49",
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
            "1": 35,
            "2": 35,
            "3": 35,
            "4": 35,
            "5": 35,
            "6": 35,
            "7": 35,
            "8": 35
          }
        },
        "36": {
          "tcvrId": 36,
          "accessControl": {
            "controllerId": "36",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_50/xcvr_reset_50",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_50/xcvr_present_50",
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
            "1": 36,
            "2": 36,
            "3": 36,
            "4": 36,
            "5": 36,
            "6": 36,
            "7": 36,
            "8": 36
          }
        },
        "37": {
          "tcvrId": 37,
          "accessControl": {
            "controllerId": "37",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_51/xcvr_reset_51",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_51/xcvr_present_51",
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
            "1": 37,
            "2": 37,
            "3": 37,
            "4": 37,
            "5": 37,
            "6": 37,
            "7": 37,
            "8": 37
          }
        },
        "38": {
          "tcvrId": 38,
          "accessControl": {
            "controllerId": "38",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_52/xcvr_reset_52",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_52/xcvr_present_52",
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
            "1": 38,
            "2": 38,
            "3": 38,
            "4": 38,
            "5": 38,
            "6": 38,
            "7": 38,
            "8": 38
          }
        },
        "39": {
          "tcvrId": 39,
          "accessControl": {
            "controllerId": "39",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_105/xcvr_reset_105",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_105/xcvr_present_105",
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
            "1": 39,
            "2": 39,
            "3": 39,
            "4": 39,
            "5": 39,
            "6": 39,
            "7": 39,
            "8": 39
          }
        },
        "40": {
          "tcvrId": 40,
          "accessControl": {
            "controllerId": "40",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_106/xcvr_reset_106",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_106/xcvr_present_106",
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
            "1": 40,
            "2": 40,
            "3": 40,
            "4": 40,
            "5": 40,
            "6": 40,
            "7": 40,
            "8": 40
          }
        },
        "41": {
          "tcvrId": 41,
          "accessControl": {
            "controllerId": "41",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_11/xcvr_reset_11",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_11/xcvr_present_11",
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
            "1": 41,
            "2": 41,
            "3": 41,
            "4": 41,
            "5": 41,
            "6": 41,
            "7": 41,
            "8": 41
          }
        },
        "42": {
          "tcvrId": 42,
          "accessControl": {
            "controllerId": "42",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_12/xcvr_reset_12",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_12/xcvr_present_12",
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
            "1": 42,
            "2": 42,
            "3": 42,
            "4": 42,
            "5": 42,
            "6": 42,
            "7": 42,
            "8": 42
          }
        },
        "43": {
          "tcvrId": 43,
          "accessControl": {
            "controllerId": "43",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_53/xcvr_reset_53",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_53/xcvr_present_53",
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
            "1": 43,
            "2": 43,
            "3": 43,
            "4": 43,
            "5": 43,
            "6": 43,
            "7": 43,
            "8": 43
          }
        },
        "44": {
          "tcvrId": 44,
          "accessControl": {
            "controllerId": "44",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_54/xcvr_reset_54",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_54/xcvr_present_54",
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
            "1": 44,
            "2": 44,
            "3": 44,
            "4": 44,
            "5": 44,
            "6": 44,
            "7": 44,
            "8": 44
          }
        },
        "45": {
          "tcvrId": 45,
          "accessControl": {
            "controllerId": "45",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_55/xcvr_reset_55",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_55/xcvr_present_55",
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
            "1": 45,
            "2": 45,
            "3": 45,
            "4": 45,
            "5": 45,
            "6": 45,
            "7": 45,
            "8": 45
          }
        },
        "46": {
          "tcvrId": 46,
          "accessControl": {
            "controllerId": "46",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_56/xcvr_reset_56",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_56/xcvr_present_56",
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
            "1": 46,
            "2": 46,
            "3": 46,
            "4": 46,
            "5": 46,
            "6": 46,
            "7": 46,
            "8": 46
          }
        },
        "47": {
          "tcvrId": 47,
          "accessControl": {
            "controllerId": "47",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_107/xcvr_reset_107",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_107/xcvr_present_107",
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
            "1": 47,
            "2": 47,
            "3": 47,
            "4": 47,
            "5": 47,
            "6": 47,
            "7": 47,
            "8": 47
          }
        },
        "48": {
          "tcvrId": 48,
          "accessControl": {
            "controllerId": "48",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_108/xcvr_reset_108",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_108/xcvr_present_108",
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
            "1": 48,
            "2": 48,
            "3": 48,
            "4": 48,
            "5": 48,
            "6": 48,
            "7": 48,
            "8": 48
          }
        },
        "49": {
          "tcvrId": 49,
          "accessControl": {
            "controllerId": "49",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_13/xcvr_reset_13",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_13/xcvr_present_13",
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
            "1": 49,
            "2": 49,
            "3": 49,
            "4": 49,
            "5": 49,
            "6": 49,
            "7": 49,
            "8": 49
          }
        },
        "50": {
          "tcvrId": 50,
          "accessControl": {
            "controllerId": "50",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_14/xcvr_reset_14",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_14/xcvr_present_14",
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
            "1": 50,
            "2": 50,
            "3": 50,
            "4": 50,
            "5": 50,
            "6": 50,
            "7": 50,
            "8": 50
          }
        },
        "51": {
          "tcvrId": 51,
          "accessControl": {
            "controllerId": "51",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_57/xcvr_reset_57",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_57/xcvr_present_57",
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
            "1": 51,
            "2": 51,
            "3": 51,
            "4": 51,
            "5": 51,
            "6": 51,
            "7": 51,
            "8": 51
          }
        },
        "52": {
          "tcvrId": 52,
          "accessControl": {
            "controllerId": "52",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_58/xcvr_reset_58",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_58/xcvr_present_58",
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
            "1": 52,
            "2": 52,
            "3": 52,
            "4": 52,
            "5": 52,
            "6": 52,
            "7": 52,
            "8": 52
          }
        },
        "53": {
          "tcvrId": 53,
          "accessControl": {
            "controllerId": "53",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_59/xcvr_reset_59",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_59/xcvr_present_59",
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
            "1": 53,
            "2": 53,
            "3": 53,
            "4": 53,
            "5": 53,
            "6": 53,
            "7": 53,
            "8": 53
          }
        },
        "54": {
          "tcvrId": 54,
          "accessControl": {
            "controllerId": "54",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_60/xcvr_reset_60",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_60/xcvr_present_60",
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
            "1": 54,
            "2": 54,
            "3": 54,
            "4": 54,
            "5": 54,
            "6": 54,
            "7": 54,
            "8": 54
          }
        },
        "55": {
          "tcvrId": 55,
          "accessControl": {
            "controllerId": "55",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_109/xcvr_reset_109",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_109/xcvr_present_109",
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
            "1": 55,
            "2": 55,
            "3": 55,
            "4": 55,
            "5": 55,
            "6": 55,
            "7": 55,
            "8": 55
          }
        },
        "56": {
          "tcvrId": 56,
          "accessControl": {
            "controllerId": "56",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_110/xcvr_reset_110",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_110/xcvr_present_110",
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
            "1": 56,
            "2": 56,
            "3": 56,
            "4": 56,
            "5": 56,
            "6": 56,
            "7": 56,
            "8": 56
          }
        },
        "57": {
          "tcvrId": 57,
          "accessControl": {
            "controllerId": "57",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_15/xcvr_reset_15",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_15/xcvr_present_15",
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
            "1": 57,
            "2": 57,
            "3": 57,
            "4": 57,
            "5": 57,
            "6": 57,
            "7": 57,
            "8": 57
          }
        },
        "58": {
          "tcvrId": 58,
          "accessControl": {
            "controllerId": "58",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_16/xcvr_reset_16",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_16/xcvr_present_16",
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
            "1": 58,
            "2": 58,
            "3": 58,
            "4": 58,
            "5": 58,
            "6": 58,
            "7": 58,
            "8": 58
          }
        },
        "59": {
          "tcvrId": 59,
          "accessControl": {
            "controllerId": "59",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_61/xcvr_reset_61",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_61/xcvr_present_61",
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
            "1": 59,
            "2": 59,
            "3": 59,
            "4": 59,
            "5": 59,
            "6": 59,
            "7": 59,
            "8": 59
          }
        },
        "60": {
          "tcvrId": 60,
          "accessControl": {
            "controllerId": "60",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_62/xcvr_reset_62",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_62/xcvr_present_62",
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
            "1": 60,
            "2": 60,
            "3": 60,
            "4": 60,
            "5": 60,
            "6": 60,
            "7": 60,
            "8": 60
          }
        },
        "61": {
          "tcvrId": 61,
          "accessControl": {
            "controllerId": "61",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_63/xcvr_reset_63",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_63/xcvr_present_63",
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
            "1": 61,
            "2": 61,
            "3": 61,
            "4": 61,
            "5": 61,
            "6": 61,
            "7": 61,
            "8": 61
          }
        },
        "62": {
          "tcvrId": 62,
          "accessControl": {
            "controllerId": "62",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_64/xcvr_reset_64",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_64/xcvr_present_64",
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
            "1": 62,
            "2": 62,
            "3": 62,
            "4": 62,
            "5": 62,
            "6": 62,
            "7": 62,
            "8": 62
          }
        },
        "63": {
          "tcvrId": 63,
          "accessControl": {
            "controllerId": "63",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_111/xcvr_reset_111",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_111/xcvr_present_111",
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
            "1": 63,
            "2": 63,
            "3": 63,
            "4": 63,
            "5": 63,
            "6": 63,
            "7": 63,
            "8": 63
          }
        },
        "64": {
          "tcvrId": 64,
          "accessControl": {
            "controllerId": "64",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_112/xcvr_reset_112",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_112/xcvr_present_112",
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
            "1": 64,
            "2": 64,
            "3": 64,
            "4": 64,
            "5": 64,
            "6": 64,
            "7": 64,
            "8": 64
          }
        },
        "65": {
          "tcvrId": 65,
          "accessControl": {
            "controllerId": "65",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_17/xcvr_reset_17",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_17/xcvr_present_17",
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
            "1": 65,
            "2": 65,
            "3": 65,
            "4": 65,
            "5": 65,
            "6": 65,
            "7": 65,
            "8": 65
          }
        },
        "66": {
          "tcvrId": 66,
          "accessControl": {
            "controllerId": "66",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_18/xcvr_reset_18",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_18/xcvr_present_18",
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
            "1": 66,
            "2": 66,
            "3": 66,
            "4": 66,
            "5": 66,
            "6": 66,
            "7": 66,
            "8": 66
          }
        },
        "67": {
          "tcvrId": 67,
          "accessControl": {
            "controllerId": "67",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_65/xcvr_reset_65",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_65/xcvr_present_65",
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
            "1": 67,
            "2": 67,
            "3": 67,
            "4": 67,
            "5": 67,
            "6": 67,
            "7": 67,
            "8": 67
          }
        },
        "68": {
          "tcvrId": 68,
          "accessControl": {
            "controllerId": "68",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_66/xcvr_reset_66",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_66/xcvr_present_66",
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
            "1": 68,
            "2": 68,
            "3": 68,
            "4": 68,
            "5": 68,
            "6": 68,
            "7": 68,
            "8": 68
          }
        },
        "69": {
          "tcvrId": 69,
          "accessControl": {
            "controllerId": "69",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_67/xcvr_reset_67",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_67/xcvr_present_67",
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
            "1": 69,
            "2": 69,
            "3": 69,
            "4": 69,
            "5": 69,
            "6": 69,
            "7": 69,
            "8": 69
          }
        },
        "70": {
          "tcvrId": 70,
          "accessControl": {
            "controllerId": "70",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_68/xcvr_reset_68",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_68/xcvr_present_68",
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
            "1": 70,
            "2": 70,
            "3": 70,
            "4": 70,
            "5": 70,
            "6": 70,
            "7": 70,
            "8": 70
          }
        },
        "71": {
          "tcvrId": 71,
          "accessControl": {
            "controllerId": "71",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_113/xcvr_reset_113",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_113/xcvr_present_113",
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
            "1": 71,
            "2": 71,
            "3": 71,
            "4": 71,
            "5": 71,
            "6": 71,
            "7": 71,
            "8": 71
          }
        },
        "72": {
          "tcvrId": 72,
          "accessControl": {
            "controllerId": "72",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_114/xcvr_reset_114",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_114/xcvr_present_114",
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
            "1": 72,
            "2": 72,
            "3": 72,
            "4": 72,
            "5": 72,
            "6": 72,
            "7": 72,
            "8": 72
          }
        },
        "73": {
          "tcvrId": 73,
          "accessControl": {
            "controllerId": "73",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_19/xcvr_reset_19",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_19/xcvr_present_19",
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
            "1": 73,
            "2": 73,
            "3": 73,
            "4": 73,
            "5": 73,
            "6": 73,
            "7": 73,
            "8": 73
          }
        },
        "74": {
          "tcvrId": 74,
          "accessControl": {
            "controllerId": "74",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_20/xcvr_reset_20",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_20/xcvr_present_20",
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
            "1": 74,
            "2": 74,
            "3": 74,
            "4": 74,
            "5": 74,
            "6": 74,
            "7": 74,
            "8": 74
          }
        },
        "75": {
          "tcvrId": 75,
          "accessControl": {
            "controllerId": "75",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_69/xcvr_reset_69",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_69/xcvr_present_69",
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
            "1": 75,
            "2": 75,
            "3": 75,
            "4": 75,
            "5": 75,
            "6": 75,
            "7": 75,
            "8": 75
          }
        },
        "76": {
          "tcvrId": 76,
          "accessControl": {
            "controllerId": "76",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_70/xcvr_reset_70",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_70/xcvr_present_70",
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
            "1": 76,
            "2": 76,
            "3": 76,
            "4": 76,
            "5": 76,
            "6": 76,
            "7": 76,
            "8": 76
          }
        },
        "77": {
          "tcvrId": 77,
          "accessControl": {
            "controllerId": "77",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_71/xcvr_reset_71",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_71/xcvr_present_71",
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
            "1": 77,
            "2": 77,
            "3": 77,
            "4": 77,
            "5": 77,
            "6": 77,
            "7": 77,
            "8": 77
          }
        },
        "78": {
          "tcvrId": 78,
          "accessControl": {
            "controllerId": "78",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_72/xcvr_reset_72",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_72/xcvr_present_72",
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
            "1": 78,
            "2": 78,
            "3": 78,
            "4": 78,
            "5": 78,
            "6": 78,
            "7": 78,
            "8": 78
          }
        },
        "79": {
          "tcvrId": 79,
          "accessControl": {
            "controllerId": "79",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_115/xcvr_reset_115",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_115/xcvr_present_115",
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
            "1": 79,
            "2": 79,
            "3": 79,
            "4": 79,
            "5": 79,
            "6": 79,
            "7": 79,
            "8": 79
          }
        },
        "80": {
          "tcvrId": 80,
          "accessControl": {
            "controllerId": "80",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_116/xcvr_reset_116",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_116/xcvr_present_116",
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
            "1": 80,
            "2": 80,
            "3": 80,
            "4": 80,
            "5": 80,
            "6": 80,
            "7": 80,
            "8": 80
          }
        },
        "81": {
          "tcvrId": 81,
          "accessControl": {
            "controllerId": "81",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_21/xcvr_reset_21",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_21/xcvr_present_21",
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
            "1": 81,
            "2": 81,
            "3": 81,
            "4": 81,
            "5": 81,
            "6": 81,
            "7": 81,
            "8": 81
          }
        },
        "82": {
          "tcvrId": 82,
          "accessControl": {
            "controllerId": "82",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_22/xcvr_reset_22",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_22/xcvr_present_22",
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
            "1": 82,
            "2": 82,
            "3": 82,
            "4": 82,
            "5": 82,
            "6": 82,
            "7": 82,
            "8": 82
          }
        },
        "83": {
          "tcvrId": 83,
          "accessControl": {
            "controllerId": "83",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_73/xcvr_reset_73",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_73/xcvr_present_73",
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
            "1": 83,
            "2": 83,
            "3": 83,
            "4": 83,
            "5": 83,
            "6": 83,
            "7": 83,
            "8": 83
          }
        },
        "84": {
          "tcvrId": 84,
          "accessControl": {
            "controllerId": "84",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_74/xcvr_reset_74",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_74/xcvr_present_74",
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
            "1": 84,
            "2": 84,
            "3": 84,
            "4": 84,
            "5": 84,
            "6": 84,
            "7": 84,
            "8": 84
          }
        },
        "85": {
          "tcvrId": 85,
          "accessControl": {
            "controllerId": "85",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_75/xcvr_reset_75",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_75/xcvr_present_75",
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
            "1": 85,
            "2": 85,
            "3": 85,
            "4": 85,
            "5": 85,
            "6": 85,
            "7": 85,
            "8": 85
          }
        },
        "86": {
          "tcvrId": 86,
          "accessControl": {
            "controllerId": "86",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_76/xcvr_reset_76",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_76/xcvr_present_76",
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
            "1": 86,
            "2": 86,
            "3": 86,
            "4": 86,
            "5": 86,
            "6": 86,
            "7": 86,
            "8": 86
          }
        },
        "87": {
          "tcvrId": 87,
          "accessControl": {
            "controllerId": "87",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_117/xcvr_reset_117",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_117/xcvr_present_117",
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
            "1": 87,
            "2": 87,
            "3": 87,
            "4": 87,
            "5": 87,
            "6": 87,
            "7": 87,
            "8": 87
          }
        },
        "88": {
          "tcvrId": 88,
          "accessControl": {
            "controllerId": "88",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_118/xcvr_reset_118",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_118/xcvr_present_118",
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
            "1": 88,
            "2": 88,
            "3": 88,
            "4": 88,
            "5": 88,
            "6": 88,
            "7": 88,
            "8": 88
          }
        },
        "89": {
          "tcvrId": 89,
          "accessControl": {
            "controllerId": "89",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_23/xcvr_reset_23",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_23/xcvr_present_23",
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
            "1": 89,
            "2": 89,
            "3": 89,
            "4": 89,
            "5": 89,
            "6": 89,
            "7": 89,
            "8": 89
          }
        },
        "90": {
          "tcvrId": 90,
          "accessControl": {
            "controllerId": "90",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_24/xcvr_reset_24",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_24/xcvr_present_24",
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
            "1": 90,
            "2": 90,
            "3": 90,
            "4": 90,
            "5": 90,
            "6": 90,
            "7": 90,
            "8": 90
          }
        },
        "91": {
          "tcvrId": 91,
          "accessControl": {
            "controllerId": "91",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_77/xcvr_reset_77",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_77/xcvr_present_77",
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
            "1": 91,
            "2": 91,
            "3": 91,
            "4": 91,
            "5": 91,
            "6": 91,
            "7": 91,
            "8": 91
          }
        },
        "92": {
          "tcvrId": 92,
          "accessControl": {
            "controllerId": "92",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_78/xcvr_reset_78",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_78/xcvr_present_78",
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
            "1": 92,
            "2": 92,
            "3": 92,
            "4": 92,
            "5": 92,
            "6": 92,
            "7": 92,
            "8": 92
          }
        },
        "93": {
          "tcvrId": 93,
          "accessControl": {
            "controllerId": "93",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_79/xcvr_reset_79",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_79/xcvr_present_79",
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
            "1": 93,
            "2": 93,
            "3": 93,
            "4": 93,
            "5": 93,
            "6": 93,
            "7": 93,
            "8": 93
          }
        },
        "94": {
          "tcvrId": 94,
          "accessControl": {
            "controllerId": "94",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_80/xcvr_reset_80",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_80/xcvr_present_80",
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
            "1": 94,
            "2": 94,
            "3": 94,
            "4": 94,
            "5": 94,
            "6": 94,
            "7": 94,
            "8": 94
          }
        },
        "95": {
          "tcvrId": 95,
          "accessControl": {
            "controllerId": "95",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_119/xcvr_reset_119",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_119/xcvr_present_119",
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
            "1": 95,
            "2": 95,
            "3": 95,
            "4": 95,
            "5": 95,
            "6": 95,
            "7": 95,
            "8": 95
          }
        },
        "96": {
          "tcvrId": 96,
          "accessControl": {
            "controllerId": "96",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_120/xcvr_reset_120",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_120/xcvr_present_120",
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
            "1": 96,
            "2": 96,
            "3": 96,
            "4": 96,
            "5": 96,
            "6": 96,
            "7": 96,
            "8": 96
          }
        },
        "97": {
          "tcvrId": 97,
          "accessControl": {
            "controllerId": "97",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_25/xcvr_reset_25",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_25/xcvr_present_25",
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
            "1": 97,
            "2": 97,
            "3": 97,
            "4": 97,
            "5": 97,
            "6": 97,
            "7": 97,
            "8": 97
          }
        },
        "98": {
          "tcvrId": 98,
          "accessControl": {
            "controllerId": "98",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_26/xcvr_reset_26",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_26/xcvr_present_26",
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
            "1": 98,
            "2": 98,
            "3": 98,
            "4": 98,
            "5": 98,
            "6": 98,
            "7": 98,
            "8": 98
          }
        },
        "99": {
          "tcvrId": 99,
          "accessControl": {
            "controllerId": "99",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_81/xcvr_reset_81",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_81/xcvr_present_81",
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
            "1": 99,
            "2": 99,
            "3": 99,
            "4": 99,
            "5": 99,
            "6": 99,
            "7": 99,
            "8": 99
          }
        },
        "100": {
          "tcvrId": 100,
          "accessControl": {
            "controllerId": "100",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_82/xcvr_reset_82",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_82/xcvr_present_82",
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
            "1": 100,
            "2": 100,
            "3": 100,
            "4": 100,
            "5": 100,
            "6": 100,
            "7": 100,
            "8": 100
          }
        },
        "101": {
          "tcvrId": 101,
          "accessControl": {
            "controllerId": "101",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_83/xcvr_reset_83",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_83/xcvr_present_83",
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
            "1": 101,
            "2": 101,
            "3": 101,
            "4": 101,
            "5": 101,
            "6": 101,
            "7": 101,
            "8": 101
          }
        },
        "102": {
          "tcvrId": 102,
          "accessControl": {
            "controllerId": "102",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_84/xcvr_reset_84",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_84/xcvr_present_84",
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
            "1": 102,
            "2": 102,
            "3": 102,
            "4": 102,
            "5": 102,
            "6": 102,
            "7": 102,
            "8": 102
          }
        },
        "103": {
          "tcvrId": 103,
          "accessControl": {
            "controllerId": "103",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_121/xcvr_reset_121",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_121/xcvr_present_121",
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
            "1": 103,
            "2": 103,
            "3": 103,
            "4": 103,
            "5": 103,
            "6": 103,
            "7": 103,
            "8": 103
          }
        },
        "104": {
          "tcvrId": 104,
          "accessControl": {
            "controllerId": "104",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_122/xcvr_reset_122",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_122/xcvr_present_122",
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
            "1": 104,
            "2": 104,
            "3": 104,
            "4": 104,
            "5": 104,
            "6": 104,
            "7": 104,
            "8": 104
          }
        },
        "105": {
          "tcvrId": 105,
          "accessControl": {
            "controllerId": "105",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_27/xcvr_reset_27",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_27/xcvr_present_27",
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
            "1": 105,
            "2": 105,
            "3": 105,
            "4": 105,
            "5": 105,
            "6": 105,
            "7": 105,
            "8": 105
          }
        },
        "106": {
          "tcvrId": 106,
          "accessControl": {
            "controllerId": "106",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_28/xcvr_reset_28",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_28/xcvr_present_28",
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
            "1": 106,
            "2": 106,
            "3": 106,
            "4": 106,
            "5": 106,
            "6": 106,
            "7": 106,
            "8": 106
          }
        },
        "107": {
          "tcvrId": 107,
          "accessControl": {
            "controllerId": "107",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_85/xcvr_reset_85",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_85/xcvr_present_85",
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
            "1": 107,
            "2": 107,
            "3": 107,
            "4": 107,
            "5": 107,
            "6": 107,
            "7": 107,
            "8": 107
          }
        },
        "108": {
          "tcvrId": 108,
          "accessControl": {
            "controllerId": "108",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_86/xcvr_reset_86",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_86/xcvr_present_86",
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
            "1": 108,
            "2": 108,
            "3": 108,
            "4": 108,
            "5": 108,
            "6": 108,
            "7": 108,
            "8": 108
          }
        },
        "109": {
          "tcvrId": 109,
          "accessControl": {
            "controllerId": "109",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_87/xcvr_reset_87",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_87/xcvr_present_87",
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
            "1": 109,
            "2": 109,
            "3": 109,
            "4": 109,
            "5": 109,
            "6": 109,
            "7": 109,
            "8": 109
          }
        },
        "110": {
          "tcvrId": 110,
          "accessControl": {
            "controllerId": "110",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_88/xcvr_reset_88",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_88/xcvr_present_88",
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
            "1": 110,
            "2": 110,
            "3": 110,
            "4": 110,
            "5": 110,
            "6": 110,
            "7": 110,
            "8": 110
          }
        },
        "111": {
          "tcvrId": 111,
          "accessControl": {
            "controllerId": "111",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_123/xcvr_reset_123",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_123/xcvr_present_123",
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
            "1": 111,
            "2": 111,
            "3": 111,
            "4": 111,
            "5": 111,
            "6": 111,
            "7": 111,
            "8": 111
          }
        },
        "112": {
          "tcvrId": 112,
          "accessControl": {
            "controllerId": "112",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_124/xcvr_reset_124",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_124/xcvr_present_124",
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
            "1": 112,
            "2": 112,
            "3": 112,
            "4": 112,
            "5": 112,
            "6": 112,
            "7": 112,
            "8": 112
          }
        },
        "113": {
          "tcvrId": 113,
          "accessControl": {
            "controllerId": "113",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_29/xcvr_reset_29",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_29/xcvr_present_29",
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
            "1": 113,
            "2": 113,
            "3": 113,
            "4": 113,
            "5": 113,
            "6": 113,
            "7": 113,
            "8": 113
          }
        },
        "114": {
          "tcvrId": 114,
          "accessControl": {
            "controllerId": "114",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_30/xcvr_reset_30",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_30/xcvr_present_30",
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
            "1": 114,
            "2": 114,
            "3": 114,
            "4": 114,
            "5": 114,
            "6": 114,
            "7": 114,
            "8": 114
          }
        },
        "115": {
          "tcvrId": 115,
          "accessControl": {
            "controllerId": "115",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_89/xcvr_reset_89",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_89/xcvr_present_89",
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
            "1": 115,
            "2": 115,
            "3": 115,
            "4": 115,
            "5": 115,
            "6": 115,
            "7": 115,
            "8": 115
          }
        },
        "116": {
          "tcvrId": 116,
          "accessControl": {
            "controllerId": "116",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_90/xcvr_reset_90",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_90/xcvr_present_90",
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
            "1": 116,
            "2": 116,
            "3": 116,
            "4": 116,
            "5": 116,
            "6": 116,
            "7": 116,
            "8": 116
          }
        },
        "117": {
          "tcvrId": 117,
          "accessControl": {
            "controllerId": "117",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_91/xcvr_reset_91",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_91/xcvr_present_91",
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
            "1": 117,
            "2": 117,
            "3": 117,
            "4": 117,
            "5": 117,
            "6": 117,
            "7": 117,
            "8": 117
          }
        },
        "118": {
          "tcvrId": 118,
          "accessControl": {
            "controllerId": "118",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_92/xcvr_reset_92",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_92/xcvr_present_92",
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
            "1": 118,
            "2": 118,
            "3": 118,
            "4": 118,
            "5": 118,
            "6": 118,
            "7": 118,
            "8": 118
          }
        },
        "119": {
          "tcvrId": 119,
          "accessControl": {
            "controllerId": "119",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_125/xcvr_reset_125",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_125/xcvr_present_125",
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
            "1": 119,
            "2": 119,
            "3": 119,
            "4": 119,
            "5": 119,
            "6": 119,
            "7": 119,
            "8": 119
          }
        },
        "120": {
          "tcvrId": 120,
          "accessControl": {
            "controllerId": "120",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_126/xcvr_reset_126",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_126/xcvr_present_126",
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
            "1": 120,
            "2": 120,
            "3": 120,
            "4": 120,
            "5": 120,
            "6": 120,
            "7": 120,
            "8": 120
          }
        },
        "121": {
          "tcvrId": 121,
          "accessControl": {
            "controllerId": "121",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_31/xcvr_reset_31",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_31/xcvr_present_31",
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
            "1": 121,
            "2": 121,
            "3": 121,
            "4": 121,
            "5": 121,
            "6": 121,
            "7": 121,
            "8": 121
          }
        },
        "122": {
          "tcvrId": 122,
          "accessControl": {
            "controllerId": "122",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_32/xcvr_reset_32",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_32/xcvr_present_32",
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
            "1": 122,
            "2": 122,
            "3": 122,
            "4": 122,
            "5": 122,
            "6": 122,
            "7": 122,
            "8": 122
          }
        },
        "123": {
          "tcvrId": 123,
          "accessControl": {
            "controllerId": "123",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_93/xcvr_reset_93",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_93/xcvr_present_93",
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
            "1": 123,
            "2": 123,
            "3": 123,
            "4": 123,
            "5": 123,
            "6": 123,
            "7": 123,
            "8": 123
          }
        },
        "124": {
          "tcvrId": 124,
          "accessControl": {
            "controllerId": "124",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_94/xcvr_reset_94",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_94/xcvr_present_94",
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
            "1": 124,
            "2": 124,
            "3": 124,
            "4": 124,
            "5": 124,
            "6": 124,
            "7": 124,
            "8": 124
          }
        },
        "125": {
          "tcvrId": 125,
          "accessControl": {
            "controllerId": "125",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_95/xcvr_reset_95",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_95/xcvr_present_95",
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
            "1": 125,
            "2": 125,
            "3": 125,
            "4": 125,
            "5": 125,
            "6": 125,
            "7": 125,
            "8": 125
          }
        },
        "126": {
          "tcvrId": 126,
          "accessControl": {
            "controllerId": "126",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_96/xcvr_reset_96",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_96/xcvr_present_96",
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
            "1": 126,
            "2": 126,
            "3": 126,
            "4": 126,
            "5": 126,
            "6": 126,
            "7": 126,
            "8": 126
          }
        },
        "127": {
          "tcvrId": 127,
          "accessControl": {
            "controllerId": "127",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_127/xcvr_reset_127",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_127/xcvr_present_127",
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
            "1": 127,
            "2": 127,
            "3": 127,
            "4": 127,
            "5": 127,
            "6": 127,
            "7": 127,
            "8": 127
          }
        },
        "128": {
          "tcvrId": 128,
          "accessControl": {
            "controllerId": "128",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_128/xcvr_reset_128",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_128/xcvr_present_128",
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
            "1": 128,
            "2": 128,
            "3": 128,
            "4": 128,
            "5": 128,
            "6": 128,
            "7": 128,
            "8": 128
          }
        },
        "129": {
          "tcvrId": 129,
          "accessControl": {
            "controllerId": "129",
            "type": 1,
            "reset": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_129/xcvr_reset_129",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 0
            },
            "presence": {
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_129/xcvr_present_129",
              "mask": 1,
              "gpioOffset": 0,
              "presentHoldHi": 0
            },
            "gpioChip": ""
          },
          "io": {
            "controllerId": "129",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_129"
          },
          "tcvrLaneToLedId": {
            "1": 129,
            "2": 129,
            "3": 129,
            "4": 129
          }
        }
      },
      "phyMapping": {},
      "phyIOControllers": {},
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port97_led1:blue:status",
          "yellowPath": "/sys/class/leds/port97_led2:amber:status",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port98_led1:blue:status",
          "yellowPath": "/sys/class/leds/port98_led2:amber:status",
          "transceiverId": 2
        },
        "3": {
          "id": 3,
          "bluePath": "/sys/class/leds/port1_led1:blue:status",
          "yellowPath": "/sys/class/leds/port1_led2:amber:status",
          "transceiverId": 3
        },
        "4": {
          "id": 4,
          "bluePath": "/sys/class/leds/port2_led1:blue:status",
          "yellowPath": "/sys/class/leds/port2_led2:amber:status",
          "transceiverId": 4
        },
        "5": {
          "id": 5,
          "bluePath": "/sys/class/leds/port33_led1:blue:status",
          "yellowPath": "/sys/class/leds/port33_led2:amber:status",
          "transceiverId": 5
        },
        "6": {
          "id": 6,
          "bluePath": "/sys/class/leds/port34_led1:blue:status",
          "yellowPath": "/sys/class/leds/port34_led2:amber:status",
          "transceiverId": 6
        },
        "7": {
          "id": 7,
          "bluePath": "/sys/class/leds/port65_led1:blue:status",
          "yellowPath": "/sys/class/leds/port65_led2:amber:status",
          "transceiverId": 7
        },
        "8": {
          "id": 8,
          "bluePath": "/sys/class/leds/port66_led1:blue:status",
          "yellowPath": "/sys/class/leds/port66_led2:amber:status",
          "transceiverId": 8
        },
        "9": {
          "id": 9,
          "bluePath": "/sys/class/leds/port99_led1:blue:status",
          "yellowPath": "/sys/class/leds/port99_led2:amber:status",
          "transceiverId": 9
        },
        "10": {
          "id": 10,
          "bluePath": "/sys/class/leds/port100_led1:blue:status",
          "yellowPath": "/sys/class/leds/port100_led2:amber:status",
          "transceiverId": 10
        },
        "11": {
          "id": 11,
          "bluePath": "/sys/class/leds/port3_led1:blue:status",
          "yellowPath": "/sys/class/leds/port3_led2:amber:status",
          "transceiverId": 11
        },
        "12": {
          "id": 12,
          "bluePath": "/sys/class/leds/port4_led1:blue:status",
          "yellowPath": "/sys/class/leds/port4_led2:amber:status",
          "transceiverId": 12
        },
        "13": {
          "id": 13,
          "bluePath": "/sys/class/leds/port35_led1:blue:status",
          "yellowPath": "/sys/class/leds/port35_led2:amber:status",
          "transceiverId": 13
        },
        "14": {
          "id": 14,
          "bluePath": "/sys/class/leds/port36_led1:blue:status",
          "yellowPath": "/sys/class/leds/port36_led2:amber:status",
          "transceiverId": 14
        },
        "15": {
          "id": 15,
          "bluePath": "/sys/class/leds/port67_led1:blue:status",
          "yellowPath": "/sys/class/leds/port67_led2:amber:status",
          "transceiverId": 15
        },
        "16": {
          "id": 16,
          "bluePath": "/sys/class/leds/port68_led1:blue:status",
          "yellowPath": "/sys/class/leds/port68_led2:amber:status",
          "transceiverId": 16
        },
        "17": {
          "id": 17,
          "bluePath": "/sys/class/leds/port101_led1:blue:status",
          "yellowPath": "/sys/class/leds/port101_led2:amber:status",
          "transceiverId": 17
        },
        "18": {
          "id": 18,
          "bluePath": "/sys/class/leds/port102_led1:blue:status",
          "yellowPath": "/sys/class/leds/port102_led2:amber:status",
          "transceiverId": 18
        },
        "19": {
          "id": 19,
          "bluePath": "/sys/class/leds/port5_led1:blue:status",
          "yellowPath": "/sys/class/leds/port5_led2:amber:status",
          "transceiverId": 19
        },
        "20": {
          "id": 20,
          "bluePath": "/sys/class/leds/port6_led1:blue:status",
          "yellowPath": "/sys/class/leds/port6_led2:amber:status",
          "transceiverId": 20
        },
        "21": {
          "id": 21,
          "bluePath": "/sys/class/leds/port37_led1:blue:status",
          "yellowPath": "/sys/class/leds/port37_led2:amber:status",
          "transceiverId": 21
        },
        "22": {
          "id": 22,
          "bluePath": "/sys/class/leds/port38_led1:blue:status",
          "yellowPath": "/sys/class/leds/port38_led2:amber:status",
          "transceiverId": 22
        },
        "23": {
          "id": 23,
          "bluePath": "/sys/class/leds/port69_led1:blue:status",
          "yellowPath": "/sys/class/leds/port69_led2:amber:status",
          "transceiverId": 23
        },
        "24": {
          "id": 24,
          "bluePath": "/sys/class/leds/port70_led1:blue:status",
          "yellowPath": "/sys/class/leds/port70_led2:amber:status",
          "transceiverId": 24
        },
        "25": {
          "id": 25,
          "bluePath": "/sys/class/leds/port103_led1:blue:status",
          "yellowPath": "/sys/class/leds/port103_led2:amber:status",
          "transceiverId": 25
        },
        "26": {
          "id": 26,
          "bluePath": "/sys/class/leds/port104_led1:blue:status",
          "yellowPath": "/sys/class/leds/port104_led2:amber:status",
          "transceiverId": 26
        },
        "27": {
          "id": 27,
          "bluePath": "/sys/class/leds/port7_led1:blue:status",
          "yellowPath": "/sys/class/leds/port7_led2:amber:status",
          "transceiverId": 27
        },
        "28": {
          "id": 28,
          "bluePath": "/sys/class/leds/port8_led1:blue:status",
          "yellowPath": "/sys/class/leds/port8_led2:amber:status",
          "transceiverId": 28
        },
        "29": {
          "id": 29,
          "bluePath": "/sys/class/leds/port39_led1:blue:status",
          "yellowPath": "/sys/class/leds/port39_led2:amber:status",
          "transceiverId": 29
        },
        "30": {
          "id": 30,
          "bluePath": "/sys/class/leds/port40_led1:blue:status",
          "yellowPath": "/sys/class/leds/port40_led2:amber:status",
          "transceiverId": 30
        },
        "31": {
          "id": 31,
          "bluePath": "/sys/class/leds/port71_led1:blue:status",
          "yellowPath": "/sys/class/leds/port71_led2:amber:status",
          "transceiverId": 31
        },
        "32": {
          "id": 32,
          "bluePath": "/sys/class/leds/port72_led1:blue:status",
          "yellowPath": "/sys/class/leds/port72_led2:amber:status",
          "transceiverId": 32
        },
        "33": {
          "id": 33,
          "bluePath": "/sys/class/leds/port105_led1:blue:status",
          "yellowPath": "/sys/class/leds/port105_led2:amber:status",
          "transceiverId": 33
        },
        "34": {
          "id": 34,
          "bluePath": "/sys/class/leds/port106_led1:blue:status",
          "yellowPath": "/sys/class/leds/port106_led2:amber:status",
          "transceiverId": 34
        },
        "35": {
          "id": 35,
          "bluePath": "/sys/class/leds/port9_led1:blue:status",
          "yellowPath": "/sys/class/leds/port9_led2:amber:status",
          "transceiverId": 35
        },
        "36": {
          "id": 36,
          "bluePath": "/sys/class/leds/port10_led1:blue:status",
          "yellowPath": "/sys/class/leds/port10_led2:amber:status",
          "transceiverId": 36
        },
        "37": {
          "id": 37,
          "bluePath": "/sys/class/leds/port41_led1:blue:status",
          "yellowPath": "/sys/class/leds/port41_led2:amber:status",
          "transceiverId": 37
        },
        "38": {
          "id": 38,
          "bluePath": "/sys/class/leds/port42_led1:blue:status",
          "yellowPath": "/sys/class/leds/port42_led2:amber:status",
          "transceiverId": 38
        },
        "39": {
          "id": 39,
          "bluePath": "/sys/class/leds/port73_led1:blue:status",
          "yellowPath": "/sys/class/leds/port73_led2:amber:status",
          "transceiverId": 39
        },
        "40": {
          "id": 40,
          "bluePath": "/sys/class/leds/port74_led1:blue:status",
          "yellowPath": "/sys/class/leds/port74_led2:amber:status",
          "transceiverId": 40
        },
        "41": {
          "id": 41,
          "bluePath": "/sys/class/leds/port107_led1:blue:status",
          "yellowPath": "/sys/class/leds/port107_led2:amber:status",
          "transceiverId": 41
        },
        "42": {
          "id": 42,
          "bluePath": "/sys/class/leds/port108_led1:blue:status",
          "yellowPath": "/sys/class/leds/port108_led2:amber:status",
          "transceiverId": 42
        },
        "43": {
          "id": 43,
          "bluePath": "/sys/class/leds/port11_led1:blue:status",
          "yellowPath": "/sys/class/leds/port11_led2:amber:status",
          "transceiverId": 43
        },
        "44": {
          "id": 44,
          "bluePath": "/sys/class/leds/port12_led1:blue:status",
          "yellowPath": "/sys/class/leds/port12_led2:amber:status",
          "transceiverId": 44
        },
        "45": {
          "id": 45,
          "bluePath": "/sys/class/leds/port43_led1:blue:status",
          "yellowPath": "/sys/class/leds/port43_led2:amber:status",
          "transceiverId": 45
        },
        "46": {
          "id": 46,
          "bluePath": "/sys/class/leds/port44_led1:blue:status",
          "yellowPath": "/sys/class/leds/port44_led2:amber:status",
          "transceiverId": 46
        },
        "47": {
          "id": 47,
          "bluePath": "/sys/class/leds/port75_led1:blue:status",
          "yellowPath": "/sys/class/leds/port75_led2:amber:status",
          "transceiverId": 47
        },
        "48": {
          "id": 48,
          "bluePath": "/sys/class/leds/port76_led1:blue:status",
          "yellowPath": "/sys/class/leds/port76_led2:amber:status",
          "transceiverId": 48
        },
        "49": {
          "id": 49,
          "bluePath": "/sys/class/leds/port109_led1:blue:status",
          "yellowPath": "/sys/class/leds/port109_led2:amber:status",
          "transceiverId": 49
        },
        "50": {
          "id": 50,
          "bluePath": "/sys/class/leds/port110_led1:blue:status",
          "yellowPath": "/sys/class/leds/port110_led2:amber:status",
          "transceiverId": 50
        },
        "51": {
          "id": 51,
          "bluePath": "/sys/class/leds/port13_led1:blue:status",
          "yellowPath": "/sys/class/leds/port13_led2:amber:status",
          "transceiverId": 51
        },
        "52": {
          "id": 52,
          "bluePath": "/sys/class/leds/port14_led1:blue:status",
          "yellowPath": "/sys/class/leds/port14_led2:amber:status",
          "transceiverId": 52
        },
        "53": {
          "id": 53,
          "bluePath": "/sys/class/leds/port45_led1:blue:status",
          "yellowPath": "/sys/class/leds/port45_led2:amber:status",
          "transceiverId": 53
        },
        "54": {
          "id": 54,
          "bluePath": "/sys/class/leds/port46_led1:blue:status",
          "yellowPath": "/sys/class/leds/port46_led2:amber:status",
          "transceiverId": 54
        },
        "55": {
          "id": 55,
          "bluePath": "/sys/class/leds/port77_led1:blue:status",
          "yellowPath": "/sys/class/leds/port77_led2:amber:status",
          "transceiverId": 55
        },
        "56": {
          "id": 56,
          "bluePath": "/sys/class/leds/port78_led1:blue:status",
          "yellowPath": "/sys/class/leds/port78_led2:amber:status",
          "transceiverId": 56
        },
        "57": {
          "id": 57,
          "bluePath": "/sys/class/leds/port111_led1:blue:status",
          "yellowPath": "/sys/class/leds/port111_led2:amber:status",
          "transceiverId": 57
        },
        "58": {
          "id": 58,
          "bluePath": "/sys/class/leds/port112_led1:blue:status",
          "yellowPath": "/sys/class/leds/port112_led2:amber:status",
          "transceiverId": 58
        },
        "59": {
          "id": 59,
          "bluePath": "/sys/class/leds/port15_led1:blue:status",
          "yellowPath": "/sys/class/leds/port15_led2:amber:status",
          "transceiverId": 59
        },
        "60": {
          "id": 60,
          "bluePath": "/sys/class/leds/port16_led1:blue:status",
          "yellowPath": "/sys/class/leds/port16_led2:amber:status",
          "transceiverId": 60
        },
        "61": {
          "id": 61,
          "bluePath": "/sys/class/leds/port47_led1:blue:status",
          "yellowPath": "/sys/class/leds/port47_led2:amber:status",
          "transceiverId": 61
        },
        "62": {
          "id": 62,
          "bluePath": "/sys/class/leds/port48_led1:blue:status",
          "yellowPath": "/sys/class/leds/port48_led2:amber:status",
          "transceiverId": 62
        },
        "63": {
          "id": 63,
          "bluePath": "/sys/class/leds/port79_led1:blue:status",
          "yellowPath": "/sys/class/leds/port79_led2:amber:status",
          "transceiverId": 63
        },
        "64": {
          "id": 64,
          "bluePath": "/sys/class/leds/port80_led1:blue:status",
          "yellowPath": "/sys/class/leds/port80_led2:amber:status",
          "transceiverId": 64
        },
        "65": {
          "id": 65,
          "bluePath": "/sys/class/leds/port113_led1:blue:status",
          "yellowPath": "/sys/class/leds/port113_led2:amber:status",
          "transceiverId": 65
        },
        "66": {
          "id": 66,
          "bluePath": "/sys/class/leds/port114_led1:blue:status",
          "yellowPath": "/sys/class/leds/port114_led2:amber:status",
          "transceiverId": 66
        },
        "67": {
          "id": 67,
          "bluePath": "/sys/class/leds/port17_led1:blue:status",
          "yellowPath": "/sys/class/leds/port17_led2:amber:status",
          "transceiverId": 67
        },
        "68": {
          "id": 68,
          "bluePath": "/sys/class/leds/port18_led1:blue:status",
          "yellowPath": "/sys/class/leds/port18_led2:amber:status",
          "transceiverId": 68
        },
        "69": {
          "id": 69,
          "bluePath": "/sys/class/leds/port49_led1:blue:status",
          "yellowPath": "/sys/class/leds/port49_led2:amber:status",
          "transceiverId": 69
        },
        "70": {
          "id": 70,
          "bluePath": "/sys/class/leds/port50_led1:blue:status",
          "yellowPath": "/sys/class/leds/port50_led2:amber:status",
          "transceiverId": 70
        },
        "71": {
          "id": 71,
          "bluePath": "/sys/class/leds/port81_led1:blue:status",
          "yellowPath": "/sys/class/leds/port81_led2:amber:status",
          "transceiverId": 71
        },
        "72": {
          "id": 72,
          "bluePath": "/sys/class/leds/port82_led1:blue:status",
          "yellowPath": "/sys/class/leds/port82_led2:amber:status",
          "transceiverId": 72
        },
        "73": {
          "id": 73,
          "bluePath": "/sys/class/leds/port115_led1:blue:status",
          "yellowPath": "/sys/class/leds/port115_led2:amber:status",
          "transceiverId": 73
        },
        "74": {
          "id": 74,
          "bluePath": "/sys/class/leds/port116_led1:blue:status",
          "yellowPath": "/sys/class/leds/port116_led2:amber:status",
          "transceiverId": 74
        },
        "75": {
          "id": 75,
          "bluePath": "/sys/class/leds/port19_led1:blue:status",
          "yellowPath": "/sys/class/leds/port19_led2:amber:status",
          "transceiverId": 75
        },
        "76": {
          "id": 76,
          "bluePath": "/sys/class/leds/port20_led1:blue:status",
          "yellowPath": "/sys/class/leds/port20_led2:amber:status",
          "transceiverId": 76
        },
        "77": {
          "id": 77,
          "bluePath": "/sys/class/leds/port51_led1:blue:status",
          "yellowPath": "/sys/class/leds/port51_led2:amber:status",
          "transceiverId": 77
        },
        "78": {
          "id": 78,
          "bluePath": "/sys/class/leds/port52_led1:blue:status",
          "yellowPath": "/sys/class/leds/port52_led2:amber:status",
          "transceiverId": 78
        },
        "79": {
          "id": 79,
          "bluePath": "/sys/class/leds/port83_led1:blue:status",
          "yellowPath": "/sys/class/leds/port83_led2:amber:status",
          "transceiverId": 79
        },
        "80": {
          "id": 80,
          "bluePath": "/sys/class/leds/port84_led1:blue:status",
          "yellowPath": "/sys/class/leds/port84_led2:amber:status",
          "transceiverId": 80
        },
        "81": {
          "id": 81,
          "bluePath": "/sys/class/leds/port117_led1:blue:status",
          "yellowPath": "/sys/class/leds/port117_led2:amber:status",
          "transceiverId": 81
        },
        "82": {
          "id": 82,
          "bluePath": "/sys/class/leds/port118_led1:blue:status",
          "yellowPath": "/sys/class/leds/port118_led2:amber:status",
          "transceiverId": 82
        },
        "83": {
          "id": 83,
          "bluePath": "/sys/class/leds/port21_led1:blue:status",
          "yellowPath": "/sys/class/leds/port21_led2:amber:status",
          "transceiverId": 83
        },
        "84": {
          "id": 84,
          "bluePath": "/sys/class/leds/port22_led1:blue:status",
          "yellowPath": "/sys/class/leds/port22_led2:amber:status",
          "transceiverId": 84
        },
        "85": {
          "id": 85,
          "bluePath": "/sys/class/leds/port53_led1:blue:status",
          "yellowPath": "/sys/class/leds/port53_led2:amber:status",
          "transceiverId": 85
        },
        "86": {
          "id": 86,
          "bluePath": "/sys/class/leds/port54_led1:blue:status",
          "yellowPath": "/sys/class/leds/port54_led2:amber:status",
          "transceiverId": 86
        },
        "87": {
          "id": 87,
          "bluePath": "/sys/class/leds/port85_led1:blue:status",
          "yellowPath": "/sys/class/leds/port85_led2:amber:status",
          "transceiverId": 87
        },
        "88": {
          "id": 88,
          "bluePath": "/sys/class/leds/port86_led1:blue:status",
          "yellowPath": "/sys/class/leds/port86_led2:amber:status",
          "transceiverId": 88
        },
        "89": {
          "id": 89,
          "bluePath": "/sys/class/leds/port119_led1:blue:status",
          "yellowPath": "/sys/class/leds/port119_led2:amber:status",
          "transceiverId": 89
        },
        "90": {
          "id": 90,
          "bluePath": "/sys/class/leds/port120_led1:blue:status",
          "yellowPath": "/sys/class/leds/port120_led2:amber:status",
          "transceiverId": 90
        },
        "91": {
          "id": 91,
          "bluePath": "/sys/class/leds/port23_led1:blue:status",
          "yellowPath": "/sys/class/leds/port23_led2:amber:status",
          "transceiverId": 91
        },
        "92": {
          "id": 92,
          "bluePath": "/sys/class/leds/port24_led1:blue:status",
          "yellowPath": "/sys/class/leds/port24_led2:amber:status",
          "transceiverId": 92
        },
        "93": {
          "id": 93,
          "bluePath": "/sys/class/leds/port55_led1:blue:status",
          "yellowPath": "/sys/class/leds/port55_led2:amber:status",
          "transceiverId": 93
        },
        "94": {
          "id": 94,
          "bluePath": "/sys/class/leds/port56_led1:blue:status",
          "yellowPath": "/sys/class/leds/port56_led2:amber:status",
          "transceiverId": 94
        },
        "95": {
          "id": 95,
          "bluePath": "/sys/class/leds/port87_led1:blue:status",
          "yellowPath": "/sys/class/leds/port87_led2:amber:status",
          "transceiverId": 95
        },
        "96": {
          "id": 96,
          "bluePath": "/sys/class/leds/port88_led1:blue:status",
          "yellowPath": "/sys/class/leds/port88_led2:amber:status",
          "transceiverId": 96
        },
        "97": {
          "id": 97,
          "bluePath": "/sys/class/leds/port121_led1:blue:status",
          "yellowPath": "/sys/class/leds/port121_led2:amber:status",
          "transceiverId": 97
        },
        "98": {
          "id": 98,
          "bluePath": "/sys/class/leds/port122_led1:blue:status",
          "yellowPath": "/sys/class/leds/port122_led2:amber:status",
          "transceiverId": 98
        },
        "99": {
          "id": 99,
          "bluePath": "/sys/class/leds/port25_led1:blue:status",
          "yellowPath": "/sys/class/leds/port25_led2:amber:status",
          "transceiverId": 99
        },
        "100": {
          "id": 100,
          "bluePath": "/sys/class/leds/port26_led1:blue:status",
          "yellowPath": "/sys/class/leds/port26_led2:amber:status",
          "transceiverId": 100
        },
        "101": {
          "id": 101,
          "bluePath": "/sys/class/leds/port57_led1:blue:status",
          "yellowPath": "/sys/class/leds/port57_led2:amber:status",
          "transceiverId": 101
        },
        "102": {
          "id": 102,
          "bluePath": "/sys/class/leds/port58_led1:blue:status",
          "yellowPath": "/sys/class/leds/port58_led2:amber:status",
          "transceiverId": 102
        },
        "103": {
          "id": 103,
          "bluePath": "/sys/class/leds/port89_led1:blue:status",
          "yellowPath": "/sys/class/leds/port89_led2:amber:status",
          "transceiverId": 103
        },
        "104": {
          "id": 104,
          "bluePath": "/sys/class/leds/port90_led1:blue:status",
          "yellowPath": "/sys/class/leds/port90_led2:amber:status",
          "transceiverId": 104
        },
        "105": {
          "id": 105,
          "bluePath": "/sys/class/leds/port123_led1:blue:status",
          "yellowPath": "/sys/class/leds/port123_led2:amber:status",
          "transceiverId": 105
        },
        "106": {
          "id": 106,
          "bluePath": "/sys/class/leds/port124_led1:blue:status",
          "yellowPath": "/sys/class/leds/port124_led2:amber:status",
          "transceiverId": 106
        },
        "107": {
          "id": 107,
          "bluePath": "/sys/class/leds/port27_led1:blue:status",
          "yellowPath": "/sys/class/leds/port27_led2:amber:status",
          "transceiverId": 107
        },
        "108": {
          "id": 108,
          "bluePath": "/sys/class/leds/port28_led1:blue:status",
          "yellowPath": "/sys/class/leds/port28_led2:amber:status",
          "transceiverId": 108
        },
        "109": {
          "id": 109,
          "bluePath": "/sys/class/leds/port59_led1:blue:status",
          "yellowPath": "/sys/class/leds/port59_led2:amber:status",
          "transceiverId": 109
        },
        "110": {
          "id": 110,
          "bluePath": "/sys/class/leds/port60_led1:blue:status",
          "yellowPath": "/sys/class/leds/port60_led2:amber:status",
          "transceiverId": 110
        },
        "111": {
          "id": 111,
          "bluePath": "/sys/class/leds/port91_led1:blue:status",
          "yellowPath": "/sys/class/leds/port91_led2:amber:status",
          "transceiverId": 111
        },
        "112": {
          "id": 112,
          "bluePath": "/sys/class/leds/port92_led1:blue:status",
          "yellowPath": "/sys/class/leds/port92_led2:amber:status",
          "transceiverId": 112
        },
        "113": {
          "id": 113,
          "bluePath": "/sys/class/leds/port125_led1:blue:status",
          "yellowPath": "/sys/class/leds/port125_led2:amber:status",
          "transceiverId": 113
        },
        "114": {
          "id": 114,
          "bluePath": "/sys/class/leds/port126_led1:blue:status",
          "yellowPath": "/sys/class/leds/port126_led2:amber:status",
          "transceiverId": 114
        },
        "115": {
          "id": 115,
          "bluePath": "/sys/class/leds/port29_led1:blue:status",
          "yellowPath": "/sys/class/leds/port29_led2:amber:status",
          "transceiverId": 115
        },
        "116": {
          "id": 116,
          "bluePath": "/sys/class/leds/port30_led1:blue:status",
          "yellowPath": "/sys/class/leds/port30_led2:amber:status",
          "transceiverId": 116
        },
        "117": {
          "id": 117,
          "bluePath": "/sys/class/leds/port61_led1:blue:status",
          "yellowPath": "/sys/class/leds/port61_led2:amber:status",
          "transceiverId": 117
        },
        "118": {
          "id": 118,
          "bluePath": "/sys/class/leds/port62_led1:blue:status",
          "yellowPath": "/sys/class/leds/port62_led2:amber:status",
          "transceiverId": 118
        },
        "119": {
          "id": 119,
          "bluePath": "/sys/class/leds/port93_led1:blue:status",
          "yellowPath": "/sys/class/leds/port93_led2:amber:status",
          "transceiverId": 119
        },
        "120": {
          "id": 120,
          "bluePath": "/sys/class/leds/port94_led1:blue:status",
          "yellowPath": "/sys/class/leds/port94_led2:amber:status",
          "transceiverId": 120
        },
        "121": {
          "id": 121,
          "bluePath": "/sys/class/leds/port127_led1:blue:status",
          "yellowPath": "/sys/class/leds/port127_led2:amber:status",
          "transceiverId": 121
        },
        "122": {
          "id": 122,
          "bluePath": "/sys/class/leds/port128_led1:blue:status",
          "yellowPath": "/sys/class/leds/port128_led2:amber:status",
          "transceiverId": 122
        },
        "123": {
          "id": 123,
          "bluePath": "/sys/class/leds/port31_led1:blue:status",
          "yellowPath": "/sys/class/leds/port31_led2:amber:status",
          "transceiverId": 123
        },
        "124": {
          "id": 124,
          "bluePath": "/sys/class/leds/port32_led1:blue:status",
          "yellowPath": "/sys/class/leds/port32_led2:amber:status",
          "transceiverId": 124
        },
        "125": {
          "id": 125,
          "bluePath": "/sys/class/leds/port63_led1:blue:status",
          "yellowPath": "/sys/class/leds/port63_led2:amber:status",
          "transceiverId": 125
        },
        "126": {
          "id": 126,
          "bluePath": "/sys/class/leds/port64_led1:blue:status",
          "yellowPath": "/sys/class/leds/port64_led2:amber:status",
          "transceiverId": 126
        },
        "127": {
          "id": 127,
          "bluePath": "/sys/class/leds/port95_led1:blue:status",
          "yellowPath": "/sys/class/leds/port95_led2:amber:status",
          "transceiverId": 127
        },
        "128": {
          "id": 128,
          "bluePath": "/sys/class/leds/port96_led1:blue:status",
          "yellowPath": "/sys/class/leds/port96_led2:amber:status",
          "transceiverId": 128
        },
        "129": {
          "id": 129,
          "bluePath": "/sys/class/leds/port129_led1:blue:status",
          "yellowPath": "/sys/class/leds/port129_led2:amber:status",
          "transceiverId": 129
        }
      }
    }
  }
})";

BspPlatformMappingThrift buildM4062nhpPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook::fboss {

M4062nhpBspPlatformMapping::M4062nhpBspPlatformMapping()
    : BspPlatformMapping(
          buildM4062nhpPlatformMapping(kJsonBspPlatformMappingStr)) {}

M4062nhpBspPlatformMapping::M4062nhpBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildM4062nhpPlatformMapping(platformMappingStr)) {}

} // namespace facebook::fboss
