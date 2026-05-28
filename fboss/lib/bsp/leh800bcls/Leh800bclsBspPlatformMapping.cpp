// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/leh800bcls/Leh800bclsBspPlatformMapping.h"
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
              "resetHoldHi": 1
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
            "4": 1
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
              "resetHoldHi": 1
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
            "4": 2
          }
        }
      },
      "phyMapping": {
        "0": {
          "phyId": 0,
          "phyIOControllerId": 1,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_1/reset_rtm_0"
        },
        "1": {
          "phyId": 1,
          "phyIOControllerId": 1,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_1/reset_rtm_0"
        },
        "2": {
          "phyId": 2,
          "phyIOControllerId": 1,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_1/reset_rtm_0"
        },
        "3": {
          "phyId": 3,
          "phyIOControllerId": 1,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_1/reset_rtm_0"
        },
        "4": {
          "phyId": 7,
          "phyIOControllerId": 2,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_2/reset_rtm_0"
        },
        "5": {
          "phyId": 6,
          "phyIOControllerId": 2,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_2/reset_rtm_0"
        },
        "6": {
          "phyId": 5,
          "phyIOControllerId": 2,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_2/reset_rtm_0"
        },
        "7": {
          "phyId": 4,
          "phyIOControllerId": 2,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_2/reset_rtm_0"
        },
        "8": {
          "phyId": 8,
          "phyIOControllerId": 3,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_3/reset_rtm_0"
        },
        "9": {
          "phyId": 9,
          "phyIOControllerId": 3,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_3/reset_rtm_0"
        },
        "10": {
          "phyId": 10,
          "phyIOControllerId": 3,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_3/reset_rtm_0"
        },
        "11": {
          "phyId": 11,
          "phyIOControllerId": 3,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_3/reset_rtm_0"
        },
        "12": {
          "phyId": 15,
          "phyIOControllerId": 4,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_4/reset_rtm_0"
        },
        "13": {
          "phyId": 14,
          "phyIOControllerId": 4,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_4/reset_rtm_0"
        },
        "14": {
          "phyId": 13,
          "phyIOControllerId": 4,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_4/reset_rtm_0"
        },
        "15": {
          "phyId": 12,
          "phyIOControllerId": 4,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_4/reset_rtm_0"
        },
        "16": {
          "phyId": 16,
          "phyIOControllerId": 5,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_5/reset_rtm_0"
        },
        "17": {
          "phyId": 17,
          "phyIOControllerId": 5,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_5/reset_rtm_0"
        },
        "18": {
          "phyId": 18,
          "phyIOControllerId": 5,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_5/reset_rtm_0"
        },
        "19": {
          "phyId": 19,
          "phyIOControllerId": 5,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_5/reset_rtm_0"
        },
        "20": {
          "phyId": 23,
          "phyIOControllerId": 6,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_6/reset_rtm_0"
        },
        "21": {
          "phyId": 22,
          "phyIOControllerId": 6,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_6/reset_rtm_0"
        },
        "22": {
          "phyId": 21,
          "phyIOControllerId": 6,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_6/reset_rtm_0"
        },
        "23": {
          "phyId": 20,
          "phyIOControllerId": 6,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_6/reset_rtm_0"
        },
        "24": {
          "phyId": 24,
          "phyIOControllerId": 7,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_7/reset_rtm_0"
        },
        "25": {
          "phyId": 25,
          "phyIOControllerId": 7,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_7/reset_rtm_0"
        },
        "26": {
          "phyId": 26,
          "phyIOControllerId": 7,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_7/reset_rtm_0"
        },
        "27": {
          "phyId": 27,
          "phyIOControllerId": 7,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_7/reset_rtm_0"
        },
        "28": {
          "phyId": 31,
          "phyIOControllerId": 8,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_8/reset_rtm_0"
        },
        "29": {
          "phyId": 30,
          "phyIOControllerId": 8,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_8/reset_rtm_0"
        },
        "30": {
          "phyId": 29,
          "phyIOControllerId": 8,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_8/reset_rtm_0"
        },
        "31": {
          "phyId": 28,
          "phyIOControllerId": 8,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_8/reset_rtm_0"
        },
        "32": {
          "phyId": 32,
          "phyIOControllerId": 9,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_9/reset_rtm_0"
        },
        "33": {
          "phyId": 33,
          "phyIOControllerId": 9,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_9/reset_rtm_0"
        },
        "34": {
          "phyId": 34,
          "phyIOControllerId": 9,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_9/reset_rtm_0"
        },
        "35": {
          "phyId": 35,
          "phyIOControllerId": 9,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_9/reset_rtm_0"
        },
        "36": {
          "phyId": 39,
          "phyIOControllerId": 10,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_10/reset_rtm_0"
        },
        "37": {
          "phyId": 38,
          "phyIOControllerId": 10,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_10/reset_rtm_0"
        },
        "38": {
          "phyId": 37,
          "phyIOControllerId": 10,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_10/reset_rtm_0"
        },
        "39": {
          "phyId": 36,
          "phyIOControllerId": 10,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_10/reset_rtm_0"
        },
        "40": {
          "phyId": 40,
          "phyIOControllerId": 11,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_11/reset_rtm_0"
        },
        "41": {
          "phyId": 41,
          "phyIOControllerId": 11,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_11/reset_rtm_0"
        },
        "42": {
          "phyId": 42,
          "phyIOControllerId": 11,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_11/reset_rtm_0"
        },
        "43": {
          "phyId": 43,
          "phyIOControllerId": 11,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_11/reset_rtm_0"
        },
        "44": {
          "phyId": 47,
          "phyIOControllerId": 12,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_12/reset_rtm_0"
        },
        "45": {
          "phyId": 46,
          "phyIOControllerId": 12,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_12/reset_rtm_0"
        },
        "46": {
          "phyId": 45,
          "phyIOControllerId": 12,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_12/reset_rtm_0"
        },
        "47": {
          "phyId": 43,
          "phyIOControllerId": 12,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_12/reset_rtm_0"
        },
        "48": {
          "phyId": 48,
          "phyIOControllerId": 13,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_13/reset_rtm_0"
        },
        "49": {
          "phyId": 49,
          "phyIOControllerId": 13,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_13/reset_rtm_0"
        },
        "50": {
          "phyId": 50,
          "phyIOControllerId": 13,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_13/reset_rtm_0"
        },
        "51": {
          "phyId": 51,
          "phyIOControllerId": 13,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_13/reset_rtm_0"
        },
        "52": {
          "phyId": 55,
          "phyIOControllerId": 14,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_14/reset_rtm_0"
        },
        "53": {
          "phyId": 54,
          "phyIOControllerId": 14,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_14/reset_rtm_0"
        },
        "54": {
          "phyId": 53,
          "phyIOControllerId": 14,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_14/reset_rtm_0"
        },
        "55": {
          "phyId": 52,
          "phyIOControllerId": 14,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_14/reset_rtm_0"
        },
        "56": {
          "phyId": 56,
          "phyIOControllerId": 15,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_1/reset_rtm_0"
        },
        "57": {
          "phyId": 57,
          "phyIOControllerId": 15,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_1/reset_rtm_0"
        },
        "58": {
          "phyId": 58,
          "phyIOControllerId": 15,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_1/reset_rtm_0"
        },
        "59": {
          "phyId": 59,
          "phyIOControllerId": 15,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_1/reset_rtm_0"
        },
        "60": {
          "phyId": 63,
          "phyIOControllerId": 16,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_2/reset_rtm_0"
        },
        "61": {
          "phyId": 62,
          "phyIOControllerId": 16,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_2/reset_rtm_0"
        },
        "62": {
          "phyId": 61,
          "phyIOControllerId": 16,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_2/reset_rtm_0"
        },
        "63": {
          "phyId": 60,
          "phyIOControllerId": 16,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_2/reset_rtm_0"
        },
        "64": {
          "phyId": 64,
          "phyIOControllerId": 17,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_3/reset_rtm_0"
        },
        "65": {
          "phyId": 65,
          "phyIOControllerId": 17,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_3/reset_rtm_0"
        },
        "66": {
          "phyId": 66,
          "phyIOControllerId": 17,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_3/reset_rtm_0"
        },
        "67": {
          "phyId": 67,
          "phyIOControllerId": 17,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_3/reset_rtm_0"
        },
        "68": {
          "phyId": 71,
          "phyIOControllerId": 18,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_4/reset_rtm_0"
        },
        "69": {
          "phyId": 70,
          "phyIOControllerId": 18,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_4/reset_rtm_0"
        },
        "70": {
          "phyId": 69,
          "phyIOControllerId": 18,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_4/reset_rtm_0"
        },
        "71": {
          "phyId": 68,
          "phyIOControllerId": 18,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_4/reset_rtm_0"
        },
        "72": {
          "phyId": 72,
          "phyIOControllerId": 19,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_5/reset_rtm_0"
        },
        "73": {
          "phyId": 73,
          "phyIOControllerId": 19,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_5/reset_rtm_0"
        },
        "74": {
          "phyId": 74,
          "phyIOControllerId": 19,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_5/reset_rtm_0"
        },
        "75": {
          "phyId": 75,
          "phyIOControllerId": 19,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_5/reset_rtm_0"
        },
        "76": {
          "phyId": 79,
          "phyIOControllerId": 20,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_6/reset_rtm_0"
        },
        "77": {
          "phyId": 78,
          "phyIOControllerId": 20,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_6/reset_rtm_0"
        },
        "78": {
          "phyId": 77,
          "phyIOControllerId": 20,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_6/reset_rtm_0"
        },
        "79": {
          "phyId": 76,
          "phyIOControllerId": 20,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_6/reset_rtm_0"
        },
        "80": {
          "phyId": 80,
          "phyIOControllerId": 21,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_7/reset_rtm_0"
        },
        "81": {
          "phyId": 81,
          "phyIOControllerId": 21,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_7/reset_rtm_0"
        },
        "82": {
          "phyId": 82,
          "phyIOControllerId": 21,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_7/reset_rtm_0"
        },
        "83": {
          "phyId": 83,
          "phyIOControllerId": 21,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_7/reset_rtm_0"
        },
        "84": {
          "phyId": 87,
          "phyIOControllerId": 22,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_8/reset_rtm_0"
        },
        "85": {
          "phyId": 86,
          "phyIOControllerId": 22,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_8/reset_rtm_0"
        },
        "86": {
          "phyId": 85,
          "phyIOControllerId": 22,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_8/reset_rtm_0"
        },
        "87": {
          "phyId": 84,
          "phyIOControllerId": 22,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_8/reset_rtm_0"
        },
        "88": {
          "phyId": 88,
          "phyIOControllerId": 23,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_9/reset_rtm_0"
        },
        "89": {
          "phyId": 89,
          "phyIOControllerId": 23,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_9/reset_rtm_0"
        },
        "90": {
          "phyId": 90,
          "phyIOControllerId": 23,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_9/reset_rtm_0"
        },
        "91": {
          "phyId": 91,
          "phyIOControllerId": 23,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_9/reset_rtm_0"
        },
        "92": {
          "phyId": 95,
          "phyIOControllerId": 24,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_10/reset_rtm_0"
        },
        "93": {
          "phyId": 94,
          "phyIOControllerId": 24,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_10/reset_rtm_0"
        },
        "94": {
          "phyId": 93,
          "phyIOControllerId": 24,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_10/reset_rtm_0"
        },
        "95": {
          "phyId": 92,
          "phyIOControllerId": 24,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_10/reset_rtm_0"
        },
        "96": {
          "phyId": 96,
          "phyIOControllerId": 25,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_11/reset_rtm_0"
        },
        "97": {
          "phyId": 97,
          "phyIOControllerId": 25,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_11/reset_rtm_0"
        },
        "98": {
          "phyId": 98,
          "phyIOControllerId": 25,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_11/reset_rtm_0"
        },
        "99": {
          "phyId": 99,
          "phyIOControllerId": 25,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_11/reset_rtm_0"
        },
        "100": {
          "phyId": 103,
          "phyIOControllerId": 26,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_12/reset_rtm_0"
        },
        "101": {
          "phyId": 102,
          "phyIOControllerId": 26,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_12/reset_rtm_0"
        },
        "102": {
          "phyId": 101,
          "phyIOControllerId": 26,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_12/reset_rtm_0"
        },
        "103": {
          "phyId": 100,
          "phyIOControllerId": 26,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_12/reset_rtm_0"
        },
        "104": {
          "phyId": 104,
          "phyIOControllerId": 27,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_13/reset_rtm_0"
        },
        "105": {
          "phyId": 105,
          "phyIOControllerId": 27,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_13/reset_rtm_0"
        },
        "106": {
          "phyId": 106,
          "phyIOControllerId": 27,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_13/reset_rtm_0"
        },
        "107": {
          "phyId": 107,
          "phyIOControllerId": 27,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_13/reset_rtm_0"
        },
        "108": {
          "phyId": 111,
          "phyIOControllerId": 28,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_14/reset_rtm_0"
        },
        "109": {
          "phyId": 110,
          "phyIOControllerId": 28,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_14/reset_rtm_0"
        },
        "110": {
          "phyId": 109,
          "phyIOControllerId": 28,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_14/reset_rtm_0"
        },
        "111": {
          "phyId": 108,
          "phyIOControllerId": 28,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_14/reset_rtm_0"
        }
      },
      "phyIOControllers": {
        "1": {
          "controllerId": 1,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_1",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_1/reset_bus"
        },
        "2": {
          "controllerId": 2,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_2",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_2/reset_bus"
        },
        "3": {
          "controllerId": 3,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_3",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_3/reset_bus"
        },
        "4": {
          "controllerId": 4,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_4",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_4/reset_bus"
        },
        "5": {
          "controllerId": 5,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_5",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_5/reset_bus"
        },
        "6": {
          "controllerId": 6,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_6",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_6/reset_bus"
        },
        "7": {
          "controllerId": 7,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_7",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_7/reset_bus"
        },
        "8": {
          "controllerId": 8,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_8",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_8/reset_bus"
        },
        "9": {
          "controllerId": 9,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_9",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_9/reset_bus"
        },
        "10": {
          "controllerId": 10,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_10",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_10/reset_bus"
        },
        "11": {
          "controllerId": 11,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_11",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_11/reset_bus"
        },
        "12": {
          "controllerId": 12,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_12",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_12/reset_bus"
        },
        "13": {
          "controllerId": 13,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_13",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_13/reset_bus"
        },
        "14": {
          "controllerId": 14,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_r_14",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_r_14/reset_bus"
        },
        "15": {
          "controllerId": 15,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_1",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_1/reset_bus"
        },
        "16": {
          "controllerId": 16,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_2",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_2/reset_bus"
        },
        "17": {
          "controllerId": 17,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_3",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_3/reset_bus"
        },
        "18": {
          "controllerId": 18,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_4",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_4/reset_bus"
        },
        "19": {
          "controllerId": 19,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_5",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_5/reset_bus"
        },
        "20": {
          "controllerId": 20,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_6",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_6/reset_bus"
        },
        "21": {
          "controllerId": 21,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_7",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_7/reset_bus"
        },
        "22": {
          "controllerId": 22,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_8",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_8/reset_bus"
        },
        "23": {
          "controllerId": 23,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_9",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_9/reset_bus"
        },
        "24": {
          "controllerId": 24,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_10",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_10/reset_bus"
        },
        "25": {
          "controllerId": 25,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_11",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_11/reset_bus"
        },
        "26": {
          "controllerId": 26,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_12",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_12/reset_bus"
        },
        "27": {
          "controllerId": 27,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_13",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_13/reset_bus"
        },
        "28": {
          "controllerId": 28,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/mdio_bus_io_l_14",
          "resetPath": "/run/devmap/mdio-busses/mdio_bus_ctrl_l_14/reset_bus"
        }
      },
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port1_led1:blue:status",
          "yellowPath": "/sys/class/leds/port1_led1:amber:status\r",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port2_led1:blue:status",
          "yellowPath": "/sys/class/leds/port2_led1:amber:status\r",
          "transceiverId": 2
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildLeh800bclsPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Leh800bclsBspPlatformMapping::Leh800bclsBspPlatformMapping()
    : BspPlatformMapping(
          buildLeh800bclsPlatformMapping(kJsonBspPlatformMappingStr)) {}

Leh800bclsBspPlatformMapping::Leh800bclsBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildLeh800bclsPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
