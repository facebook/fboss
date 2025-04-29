// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/darwin/DarwinBspPlatformMapping.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {

// This is generated from the csv under fboss/lib/bsp/bspmapping/input
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH0"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH1"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH2"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH3"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH4"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH5"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH6"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH7"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH0"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH1"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH2"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH3"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH4"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH5"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH6"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS3_CH7"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH0"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH1"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH2"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH3"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH4"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH5"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH6"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS4_CH7"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH0"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH1"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH2"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH3"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH4"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH5"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH6"
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
            "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS5_CH7"
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
        }
      },
      "phyMapping": {},
      "phyIOControllers": {},
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port1_led1:green:status",
          "yellowPath": "/sys/class/leds/port1_led1:yellow:status\r",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port2_led1:green:status",
          "yellowPath": "/sys/class/leds/port2_led1:yellow:status\r",
          "transceiverId": 2
        },
        "3": {
          "id": 3,
          "bluePath": "/sys/class/leds/port3_led1:green:status",
          "yellowPath": "/sys/class/leds/port3_led1:yellow:status\r",
          "transceiverId": 3
        },
        "4": {
          "id": 4,
          "bluePath": "/sys/class/leds/port4_led1:green:status",
          "yellowPath": "/sys/class/leds/port4_led1:yellow:status\r",
          "transceiverId": 4
        },
        "5": {
          "id": 5,
          "bluePath": "/sys/class/leds/port5_led1:green:status",
          "yellowPath": "/sys/class/leds/port5_led1:yellow:status\r",
          "transceiverId": 5
        },
        "6": {
          "id": 6,
          "bluePath": "/sys/class/leds/port6_led1:green:status",
          "yellowPath": "/sys/class/leds/port6_led1:yellow:status\r",
          "transceiverId": 6
        },
        "7": {
          "id": 7,
          "bluePath": "/sys/class/leds/port7_led1:green:status",
          "yellowPath": "/sys/class/leds/port7_led1:yellow:status\r",
          "transceiverId": 7
        },
        "8": {
          "id": 8,
          "bluePath": "/sys/class/leds/port8_led1:green:status",
          "yellowPath": "/sys/class/leds/port8_led1:yellow:status\r",
          "transceiverId": 8
        },
        "9": {
          "id": 9,
          "bluePath": "/sys/class/leds/port9_led1:green:status",
          "yellowPath": "/sys/class/leds/port9_led1:yellow:status\r",
          "transceiverId": 9
        },
        "10": {
          "id": 10,
          "bluePath": "/sys/class/leds/port10_led1:green:status",
          "yellowPath": "/sys/class/leds/port10_led1:yellow:status\r",
          "transceiverId": 10
        },
        "11": {
          "id": 11,
          "bluePath": "/sys/class/leds/port11_led1:green:status",
          "yellowPath": "/sys/class/leds/port11_led1:yellow:status\r",
          "transceiverId": 11
        },
        "12": {
          "id": 12,
          "bluePath": "/sys/class/leds/port12_led1:green:status",
          "yellowPath": "/sys/class/leds/port12_led1:yellow:status\r",
          "transceiverId": 12
        },
        "13": {
          "id": 13,
          "bluePath": "/sys/class/leds/port13_led1:green:status",
          "yellowPath": "/sys/class/leds/port13_led1:yellow:status\r",
          "transceiverId": 13
        },
        "14": {
          "id": 14,
          "bluePath": "/sys/class/leds/port14_led1:green:status",
          "yellowPath": "/sys/class/leds/port14_led1:yellow:status\r",
          "transceiverId": 14
        },
        "15": {
          "id": 15,
          "bluePath": "/sys/class/leds/port15_led1:green:status",
          "yellowPath": "/sys/class/leds/port15_led1:yellow:status\r",
          "transceiverId": 15
        },
        "16": {
          "id": 16,
          "bluePath": "/sys/class/leds/port16_led1:green:status",
          "yellowPath": "/sys/class/leds/port16_led1:yellow:status\r",
          "transceiverId": 16
        },
        "17": {
          "id": 17,
          "bluePath": "/sys/class/leds/port17_led1:green:status",
          "yellowPath": "/sys/class/leds/port17_led1:yellow:status\r",
          "transceiverId": 17
        },
        "18": {
          "id": 18,
          "bluePath": "/sys/class/leds/port18_led1:green:status",
          "yellowPath": "/sys/class/leds/port18_led1:yellow:status\r",
          "transceiverId": 18
        },
        "19": {
          "id": 19,
          "bluePath": "/sys/class/leds/port19_led1:green:status",
          "yellowPath": "/sys/class/leds/port19_led1:yellow:status\r",
          "transceiverId": 19
        },
        "20": {
          "id": 20,
          "bluePath": "/sys/class/leds/port20_led1:green:status",
          "yellowPath": "/sys/class/leds/port20_led1:yellow:status\r",
          "transceiverId": 20
        },
        "21": {
          "id": 21,
          "bluePath": "/sys/class/leds/port21_led1:green:status",
          "yellowPath": "/sys/class/leds/port21_led1:yellow:status\r",
          "transceiverId": 21
        },
        "22": {
          "id": 22,
          "bluePath": "/sys/class/leds/port22_led1:green:status",
          "yellowPath": "/sys/class/leds/port22_led1:yellow:status\r",
          "transceiverId": 22
        },
        "23": {
          "id": 23,
          "bluePath": "/sys/class/leds/port23_led1:green:status",
          "yellowPath": "/sys/class/leds/port23_led1:yellow:status\r",
          "transceiverId": 23
        },
        "24": {
          "id": 24,
          "bluePath": "/sys/class/leds/port24_led1:green:status",
          "yellowPath": "/sys/class/leds/port24_led1:yellow:status\r",
          "transceiverId": 24
        },
        "25": {
          "id": 25,
          "bluePath": "/sys/class/leds/port25_led1:green:status",
          "yellowPath": "/sys/class/leds/port25_led1:yellow:status\r",
          "transceiverId": 25
        },
        "26": {
          "id": 26,
          "bluePath": "/sys/class/leds/port26_led1:green:status",
          "yellowPath": "/sys/class/leds/port26_led1:yellow:status\r",
          "transceiverId": 26
        },
        "27": {
          "id": 27,
          "bluePath": "/sys/class/leds/port27_led1:green:status",
          "yellowPath": "/sys/class/leds/port27_led1:yellow:status\r",
          "transceiverId": 27
        },
        "28": {
          "id": 28,
          "bluePath": "/sys/class/leds/port28_led1:green:status",
          "yellowPath": "/sys/class/leds/port28_led1:yellow:status\r",
          "transceiverId": 28
        },
        "29": {
          "id": 29,
          "bluePath": "/sys/class/leds/port29_led1:green:status",
          "yellowPath": "/sys/class/leds/port29_led1:yellow:status\r",
          "transceiverId": 29
        },
        "30": {
          "id": 30,
          "bluePath": "/sys/class/leds/port30_led1:green:status",
          "yellowPath": "/sys/class/leds/port30_led1:yellow:status\r",
          "transceiverId": 30
        },
        "31": {
          "id": 31,
          "bluePath": "/sys/class/leds/port31_led1:green:status",
          "yellowPath": "/sys/class/leds/port31_led1:yellow:status\r",
          "transceiverId": 31
        },
        "32": {
          "id": 32,
          "bluePath": "/sys/class/leds/port32_led1:green:status",
          "yellowPath": "/sys/class/leds/port32_led1:yellow:status",
          "transceiverId": 32
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildDarwinPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

DarwinBspPlatformMapping::DarwinBspPlatformMapping()
    : BspPlatformMapping(
          buildDarwinPlatformMapping(kJsonBspPlatformMappingStr)) {}

DarwinBspPlatformMapping::DarwinBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildDarwinPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
