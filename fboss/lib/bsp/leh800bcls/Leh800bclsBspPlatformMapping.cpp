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
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_1_1/rtm_reset_57"
        },
        "1": {
          "phyId": 1,          "phyIOControllerId": 1,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_1_2/rtm_reset_58"
        },
        "2": {
          "phyId": 2,          "phyIOControllerId": 1,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_1_3/rtm_reset_59"
        },
        "3": {
          "phyId": 3,          "phyIOControllerId": 1,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_1_4/rtm_reset_60"
        },
        "4": {
          "phyId": 4,
          "phyIOControllerId": 2,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_2_4/rtm_reset_64"
        },
        "5": {
          "phyId": 5,
          "phyIOControllerId": 2,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_2_3/rtm_reset_63"
        },
        "6": {
          "phyId": 6,
          "phyIOControllerId": 2,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_2_2/rtm_reset_62"
        },
        "7": {
          "phyId": 7,
          "phyIOControllerId": 2,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_2_1/rtm_reset_61"
        },
        "8": {
          "phyId": 8,          "phyIOControllerId": 3,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_3_1/rtm_reset_65"
        },
        "9": {
          "phyId": 9,          "phyIOControllerId": 3,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_3_2/rtm_reset_66"
        },
        "10": {
          "phyId": 10,          "phyIOControllerId": 3,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_3_3/rtm_reset_67"
        },
        "11": {
          "phyId": 11,          "phyIOControllerId": 3,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_3_4/rtm_reset_68"
        },
        "12": {
          "phyId": 12,
          "phyIOControllerId": 4,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_4_4/rtm_reset_72"
        },
        "13": {
          "phyId": 13,
          "phyIOControllerId": 4,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_4_3/rtm_reset_71"
        },
        "14": {
          "phyId": 14,
          "phyIOControllerId": 4,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_4_2/rtm_reset_70"
        },
        "15": {
          "phyId": 15,
          "phyIOControllerId": 4,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_4_1/rtm_reset_69"
        },
        "16": {
          "phyId": 16,          "phyIOControllerId": 5,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_5_1/rtm_reset_73"
        },
        "17": {
          "phyId": 17,          "phyIOControllerId": 5,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_5_2/rtm_reset_74"
        },
        "18": {
          "phyId": 18,          "phyIOControllerId": 5,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_5_3/rtm_reset_75"
        },
        "19": {
          "phyId": 19,          "phyIOControllerId": 5,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_5_4/rtm_reset_76"
        },
        "20": {
          "phyId": 20,
          "phyIOControllerId": 6,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_6_4/rtm_reset_80"
        },
        "21": {
          "phyId": 21,
          "phyIOControllerId": 6,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_6_3/rtm_reset_79"
        },
        "22": {
          "phyId": 22,
          "phyIOControllerId": 6,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_6_2/rtm_reset_78"
        },
        "23": {
          "phyId": 23,
          "phyIOControllerId": 6,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_6_1/rtm_reset_77"
        },
        "24": {
          "phyId": 24,          "phyIOControllerId": 7,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_7_1/rtm_reset_81"
        },
        "25": {
          "phyId": 25,          "phyIOControllerId": 7,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_7_2/rtm_reset_82"
        },
        "26": {
          "phyId": 26,          "phyIOControllerId": 7,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_7_3/rtm_reset_83"
        },
        "27": {
          "phyId": 27,          "phyIOControllerId": 7,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_7_4/rtm_reset_84"
        },
        "28": {
          "phyId": 28,
          "phyIOControllerId": 8,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_8_4/rtm_reset_88"
        },
        "29": {
          "phyId": 29,
          "phyIOControllerId": 8,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_8_3/rtm_reset_87"
        },
        "30": {
          "phyId": 30,
          "phyIOControllerId": 8,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_8_2/rtm_reset_86"
        },
        "31": {
          "phyId": 31,
          "phyIOControllerId": 8,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_8_1/rtm_reset_85"
        },
        "32": {
          "phyId": 32,          "phyIOControllerId": 9,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_9_1/rtm_reset_89"
        },
        "33": {
          "phyId": 33,          "phyIOControllerId": 9,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_9_2/rtm_reset_90"
        },
        "34": {
          "phyId": 34,          "phyIOControllerId": 9,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_9_3/rtm_reset_91"
        },
        "35": {
          "phyId": 35,          "phyIOControllerId": 9,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_9_4/rtm_reset_92"
        },
        "36": {
          "phyId": 36,
          "phyIOControllerId": 10,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_10_4/rtm_reset_96"
        },
        "37": {
          "phyId": 37,
          "phyIOControllerId": 10,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_10_3/rtm_reset_95"
        },
        "38": {
          "phyId": 38,
          "phyIOControllerId": 10,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_10_2/rtm_reset_94"
        },
        "39": {
          "phyId": 39,
          "phyIOControllerId": 10,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_10_1/rtm_reset_93"
        },
        "40": {
          "phyId": 40,          "phyIOControllerId": 11,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_11_1/rtm_reset_97"
        },
        "41": {
          "phyId": 41,          "phyIOControllerId": 11,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_11_2/rtm_reset_98"
        },
        "42": {
          "phyId": 42,          "phyIOControllerId": 11,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_11_3/rtm_reset_99"
        },
        "43": {
          "phyId": 43,          "phyIOControllerId": 11,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_11_4/rtm_reset_100"
        },
        "44": {
          "phyId": 44,
          "phyIOControllerId": 12,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_12_4/rtm_reset_104"
        },
        "45": {
          "phyId": 45,
          "phyIOControllerId": 12,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_12_3/rtm_reset_103"
        },
        "46": {
          "phyId": 46,
          "phyIOControllerId": 12,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_12_2/rtm_reset_102"
        },
        "47": {
          "phyId": 47,
          "phyIOControllerId": 12,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_12_1/rtm_reset_101"
        },
        "48": {
          "phyId": 48,          "phyIOControllerId": 13,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_13_1/rtm_reset_105"
        },
        "49": {
          "phyId": 49,          "phyIOControllerId": 13,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_13_2/rtm_reset_106"
        },
        "50": {
          "phyId": 50,          "phyIOControllerId": 13,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_13_3/rtm_reset_107"
        },
        "51": {
          "phyId": 51,          "phyIOControllerId": 13,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_13_4/rtm_reset_108"
        },
        "52": {
          "phyId": 52,
          "phyIOControllerId": 14,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_14_4/rtm_reset_112"
        },
        "53": {
          "phyId": 53,
          "phyIOControllerId": 14,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_14_3/rtm_reset_111"
        },
        "54": {
          "phyId": 54,
          "phyIOControllerId": 14,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_14_2/rtm_reset_110"
        },
        "55": {
          "phyId": 55,
          "phyIOControllerId": 14,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_r_14_1/rtm_reset_109"
        },
        "56": {
          "phyId": 56,          "phyIOControllerId": 15,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_1_1/rtm_reset_1"
        },
        "57": {
          "phyId": 57,          "phyIOControllerId": 15,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_1_2/rtm_reset_2"
        },
        "58": {
          "phyId": 58,          "phyIOControllerId": 15,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_1_3/rtm_reset_3"
        },
        "59": {
          "phyId": 59,          "phyIOControllerId": 15,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_1_4/rtm_reset_4"
        },
        "60": {
          "phyId": 60,
          "phyIOControllerId": 16,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_2_4/rtm_reset_8"
        },
        "61": {
          "phyId": 61,
          "phyIOControllerId": 16,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_2_3/rtm_reset_7"
        },
        "62": {
          "phyId": 62,
          "phyIOControllerId": 16,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_2_2/rtm_reset_6"
        },
        "63": {
          "phyId": 63,
          "phyIOControllerId": 16,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_2_1/rtm_reset_5"
        },
        "64": {
          "phyId": 64,          "phyIOControllerId": 17,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_3_1/rtm_reset_9"
        },
        "65": {
          "phyId": 65,          "phyIOControllerId": 17,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_3_2/rtm_reset_10"
        },
        "66": {
          "phyId": 66,          "phyIOControllerId": 17,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_3_3/rtm_reset_11"
        },
        "67": {
          "phyId": 67,          "phyIOControllerId": 17,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_3_4/rtm_reset_12"
        },
        "68": {
          "phyId": 68,
          "phyIOControllerId": 18,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_4_4/rtm_reset_16"
        },
        "69": {
          "phyId": 69,
          "phyIOControllerId": 18,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_4_3/rtm_reset_15"
        },
        "70": {
          "phyId": 70,
          "phyIOControllerId": 18,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_4_2/rtm_reset_14"
        },
        "71": {
          "phyId": 71,
          "phyIOControllerId": 18,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_4_1/rtm_reset_13"
        },
        "72": {
          "phyId": 72,          "phyIOControllerId": 19,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_5_1/rtm_reset_17"
        },
        "73": {
          "phyId": 73,          "phyIOControllerId": 19,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_5_2/rtm_reset_18"
        },
        "74": {
          "phyId": 74,          "phyIOControllerId": 19,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_5_3/rtm_reset_19"
        },
        "75": {
          "phyId": 75,          "phyIOControllerId": 19,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_5_4/rtm_reset_20"
        },
        "76": {
          "phyId": 76,
          "phyIOControllerId": 20,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_6_4/rtm_reset_24"
        },
        "77": {
          "phyId": 77,
          "phyIOControllerId": 20,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_6_3/rtm_reset_23"
        },
        "78": {
          "phyId": 78,
          "phyIOControllerId": 20,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_6_2/rtm_reset_22"
        },
        "79": {
          "phyId": 79,
          "phyIOControllerId": 20,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_6_1/rtm_reset_21"
        },
        "80": {
          "phyId": 80,          "phyIOControllerId": 21,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_7_1/rtm_reset_25"
        },
        "81": {
          "phyId": 81,          "phyIOControllerId": 21,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_7_2/rtm_reset_26"
        },
        "82": {
          "phyId": 82,          "phyIOControllerId": 21,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_7_3/rtm_reset_27"
        },
        "83": {
          "phyId": 83,          "phyIOControllerId": 21,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_7_4/rtm_reset_28"
        },
        "84": {
          "phyId": 84,
          "phyIOControllerId": 22,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_8_4/rtm_reset_32"
        },
        "85": {
          "phyId": 85,
          "phyIOControllerId": 22,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_8_3/rtm_reset_31"
        },
        "86": {
          "phyId": 86,
          "phyIOControllerId": 22,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_8_2/rtm_reset_30"
        },
        "87": {
          "phyId": 87,
          "phyIOControllerId": 22,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_8_1/rtm_reset_29"
        },
        "88": {
          "phyId": 88,          "phyIOControllerId": 23,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_9_1/rtm_reset_33"
        },
        "89": {
          "phyId": 89,          "phyIOControllerId": 23,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_9_2/rtm_reset_34"
        },
        "90": {
          "phyId": 90,          "phyIOControllerId": 23,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_9_3/rtm_reset_35"
        },
        "91": {
          "phyId": 91,          "phyIOControllerId": 23,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_9_4/rtm_reset_36"
        },
        "92": {
          "phyId": 92,
          "phyIOControllerId": 24,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_10_4/rtm_reset_40"
        },
        "93": {
          "phyId": 93,
          "phyIOControllerId": 24,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_10_3/rtm_reset_39"
        },
        "94": {
          "phyId": 94,
          "phyIOControllerId": 24,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_10_2/rtm_reset_38"
        },
        "95": {
          "phyId": 95,
          "phyIOControllerId": 24,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_10_1/rtm_reset_37"
        },
        "96": {
          "phyId": 96,          "phyIOControllerId": 25,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_11_1/rtm_reset_41"
        },
        "97": {
          "phyId": 97,          "phyIOControllerId": 25,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_11_2/rtm_reset_42"
        },
        "98": {
          "phyId": 98,          "phyIOControllerId": 25,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_11_3/rtm_reset_43"
        },
        "99": {
          "phyId": 99,          "phyIOControllerId": 25,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_11_4/rtm_reset_44"
        },
        "100": {
          "phyId": 100,
          "phyIOControllerId": 26,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_12_4/rtm_reset_48"
        },
        "101": {
          "phyId": 101,
          "phyIOControllerId": 26,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_12_3/rtm_reset_47"
        },
        "102": {
          "phyId": 102,
          "phyIOControllerId": 26,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_12_2/rtm_reset_46"
        },
        "103": {
          "phyId": 103,
          "phyIOControllerId": 26,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_12_1/rtm_reset_45"
        },
        "104": {
          "phyId": 104,          "phyIOControllerId": 27,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_13_1/rtm_reset_49"
        },
        "105": {
          "phyId": 105,          "phyIOControllerId": 27,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_13_2/rtm_reset_50"
        },
        "106": {
          "phyId": 106,          "phyIOControllerId": 27,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_13_3/rtm_reset_51"
        },
        "107": {
          "phyId": 107,          "phyIOControllerId": 27,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_13_4/rtm_reset_52"
        },
        "108": {
          "phyId": 108,
          "phyIOControllerId": 28,
          "phyAddr": 8,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_14_4/rtm_reset_56"
        },
        "109": {
          "phyId": 109,
          "phyIOControllerId": 28,
          "phyAddr": 4,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_14_3/rtm_reset_55"
        },
        "110": {
          "phyId": 110,
          "phyIOControllerId": 28,
          "phyAddr": 2,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_14_2/rtm_reset_54"
        },
        "111": {
          "phyId": 111,
          "phyIOControllerId": 28,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": "/run/devmap/rtms/rtm_ctrl_mdio_l_14_1/rtm_reset_53"
        }
      },
      "phyIOControllers": {        "1": {
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
