// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/ladakh800bcls/Ladakh800bclsBspPlatformMapping.h"
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
          "phyResetPath": ""
        },
        "1": {
          "phyId": 1,
          "phyIOControllerId": 2,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "2": {
          "phyId": 2,
          "phyIOControllerId": 3,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "3": {
          "phyId": 3,
          "phyIOControllerId": 4,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "4": {
          "phyId": 4,
          "phyIOControllerId": 5,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "5": {
          "phyId": 5,
          "phyIOControllerId": 6,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "6": {
          "phyId": 6,
          "phyIOControllerId": 7,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "7": {
          "phyId": 7,
          "phyIOControllerId": 8,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "8": {
          "phyId": 8,
          "phyIOControllerId": 9,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "9": {
          "phyId": 9,
          "phyIOControllerId": 10,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "10": {
          "phyId": 10,
          "phyIOControllerId": 11,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "11": {
          "phyId": 11,
          "phyIOControllerId": 12,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "12": {
          "phyId": 12,
          "phyIOControllerId": 13,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "13": {
          "phyId": 13,
          "phyIOControllerId": 14,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "14": {
          "phyId": 14,
          "phyIOControllerId": 15,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "15": {
          "phyId": 15,
          "phyIOControllerId": 16,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "16": {
          "phyId": 16,
          "phyIOControllerId": 17,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "17": {
          "phyId": 17,
          "phyIOControllerId": 18,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "18": {
          "phyId": 18,
          "phyIOControllerId": 19,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "19": {
          "phyId": 19,
          "phyIOControllerId": 20,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "20": {
          "phyId": 20,
          "phyIOControllerId": 21,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "21": {
          "phyId": 21,
          "phyIOControllerId": 22,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "22": {
          "phyId": 22,
          "phyIOControllerId": 23,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "23": {
          "phyId": 23,
          "phyIOControllerId": 24,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "24": {
          "phyId": 24,
          "phyIOControllerId": 25,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "25": {
          "phyId": 25,
          "phyIOControllerId": 26,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "26": {
          "phyId": 26,
          "phyIOControllerId": 27,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "27": {
          "phyId": 27,
          "phyIOControllerId": 28,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "28": {
          "phyId": 28,
          "phyIOControllerId": 29,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "29": {
          "phyId": 29,
          "phyIOControllerId": 30,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "30": {
          "phyId": 30,
          "phyIOControllerId": 31,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        },
        "31": {
          "phyId": 31,
          "phyIOControllerId": 32,
          "phyAddr": 0,
          "phyCoreId": 0,
          "pimId": 1,
          "phyResetPath": ""
        }
      },
      "phyIOControllers": {
        "1": {
          "controllerId": 1,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_1",
          "resetPath": ""
        },
        "2": {
          "controllerId": 2,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_2",
          "resetPath": ""
        },
        "3": {
          "controllerId": 3,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_3",
          "resetPath": ""
        },
        "4": {
          "controllerId": 4,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_4",
          "resetPath": ""
        },
        "5": {
          "controllerId": 5,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_5",
          "resetPath": ""
        },
        "6": {
          "controllerId": 6,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_6",
          "resetPath": ""
        },
        "7": {
          "controllerId": 7,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_7",
          "resetPath": ""
        },
        "8": {
          "controllerId": 8,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_8",
          "resetPath": ""
        },
        "9": {
          "controllerId": 9,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_9",
          "resetPath": ""
        },
        "10": {
          "controllerId": 10,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_10",
          "resetPath": ""
        },
        "11": {
          "controllerId": 11,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_11",
          "resetPath": ""
        },
        "12": {
          "controllerId": 12,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_12",
          "resetPath": ""
        },
        "13": {
          "controllerId": 13,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_13",
          "resetPath": ""
        },
        "14": {
          "controllerId": 14,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_14",
          "resetPath": ""
        },
        "15": {
          "controllerId": 15,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_15",
          "resetPath": ""
        },
        "16": {
          "controllerId": 16,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_L_MDIO_BUS_16",
          "resetPath": ""
        },
        "17": {
          "controllerId": 17,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_1",
          "resetPath": ""
        },
        "18": {
          "controllerId": 18,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_2",
          "resetPath": ""
        },
        "19": {
          "controllerId": 19,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_3",
          "resetPath": ""
        },
        "20": {
          "controllerId": 20,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_4",
          "resetPath": ""
        },
        "21": {
          "controllerId": 21,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_5",
          "resetPath": ""
        },
        "22": {
          "controllerId": 22,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_6",
          "resetPath": ""
        },
        "23": {
          "controllerId": 23,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_7",
          "resetPath": ""
        },
        "24": {
          "controllerId": 24,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_8",
          "resetPath": ""
        },
        "25": {
          "controllerId": 25,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_9",
          "resetPath": ""
        },
        "26": {
          "controllerId": 26,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_10",
          "resetPath": ""
        },
        "27": {
          "controllerId": 27,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_11",
          "resetPath": ""
        },
        "28": {
          "controllerId": 28,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_12",
          "resetPath": ""
        },
        "29": {
          "controllerId": 29,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_13",
          "resetPath": ""
        },
        "30": {
          "controllerId": 30,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_14",
          "resetPath": ""
        },
        "31": {
          "controllerId": 31,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_15",
          "resetPath": ""
        },
        "32": {
          "controllerId": 32,
          "type": 1,
          "devicePath": "/run/devmap/mdio-busses/RTM_R_MDIO_BUS_16",
          "resetPath": ""
        }
      },
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port1_led1:blue:status",
          "yellowPath": "/sys/class/leds/port1_led1:amber:status",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port2_led1:blue:status",
          "yellowPath": "/sys/class/leds/port2_led1:amber:status",
          "transceiverId": 2
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildLadakh800bclsPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Ladakh800bclsBspPlatformMapping::Ladakh800bclsBspPlatformMapping()
    : BspPlatformMapping(
          buildLadakh800bclsPlatformMapping(kJsonBspPlatformMappingStr)) {}

Ladakh800bclsBspPlatformMapping::Ladakh800bclsBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          buildLadakh800bclsPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
