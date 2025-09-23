// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/tahansb800bc/Tahansb800bcBspPlatformMapping.h"
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr_reset_3",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_4/xcvr_reset_4",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_5/xcvr_reset_5",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_6/xcvr_reset_6",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_7/xcvr_reset_7",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_8/xcvr_reset_8",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
              "sysfsPath": "/run/devmap/xcvrs/xcvr_ctrl_9/xcvr_reset_9",
              "mask": 1,
              "gpioOffset": 0,
              "resetHoldHi": 1
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
            "controllerId": "9",
            "type": 1,
            "devicePath": "/run/devmap/xcvrs/xcvr_io_9"
          },
          "tcvrLaneToLedId": {
            "1": 17,
            "2": 17,
            "3": 17,
            "4": 17
          }
        }
      },
      "phyMapping": {},
      "phyIOControllers": {},
      "ledMapping": {
        "1": {
          "id": 1,
          "bluePath": "/sys/class/leds/port1_led1:blue:status",
          "yellowPath": "/sys/class/leds/port1_led1:yellow:status\r",
          "transceiverId": 1
        },
        "2": {
          "id": 2,
          "bluePath": "/sys/class/leds/port1_led2:blue:status",
          "yellowPath": "/sys/class/leds/port1_led2:yellow:status\r",
          "transceiverId": 1
        },
        "3": {
          "id": 3,
          "bluePath": "/sys/class/leds/port2_led1:blue:status",
          "yellowPath": "/sys/class/leds/port2_led1:yellow:status\r",
          "transceiverId": 2
        },
        "4": {
          "id": 4,
          "bluePath": "/sys/class/leds/port2_led2:blue:status",
          "yellowPath": "/sys/class/leds/port2_led2:yellow:status\r",
          "transceiverId": 2
        },
        "5": {
          "id": 5,
          "bluePath": "/sys/class/leds/port3_led1:blue:status",
          "yellowPath": "/sys/class/leds/port3_led1:yellow:status\r",
          "transceiverId": 3
        },
        "6": {
          "id": 6,
          "bluePath": "/sys/class/leds/port3_led2:blue:status",
          "yellowPath": "/sys/class/leds/port3_led2:yellow:status\r",
          "transceiverId": 3
        },
        "7": {
          "id": 7,
          "bluePath": "/sys/class/leds/port4_led1:blue:status",
          "yellowPath": "/sys/class/leds/port4_led1:yellow:status\r",
          "transceiverId": 4
        },
        "8": {
          "id": 8,
          "bluePath": "/sys/class/leds/port4_led2:blue:status",
          "yellowPath": "/sys/class/leds/port4_led2:yellow:status\r",
          "transceiverId": 4
        },
        "9": {
          "id": 9,
          "bluePath": "/sys/class/leds/port5_led1:blue:status",
          "yellowPath": "/sys/class/leds/port5_led1:yellow:status\r",
          "transceiverId": 5
        },
        "10": {
          "id": 10,
          "bluePath": "/sys/class/leds/port5_led2:blue:status",
          "yellowPath": "/sys/class/leds/port5_led2:yellow:status\r",
          "transceiverId": 5
        },
        "11": {
          "id": 11,
          "bluePath": "/sys/class/leds/port6_led1:blue:status",
          "yellowPath": "/sys/class/leds/port6_led1:yellow:status\r",
          "transceiverId": 6
        },
        "12": {
          "id": 12,
          "bluePath": "/sys/class/leds/port6_led2:blue:status",
          "yellowPath": "/sys/class/leds/port6_led2:yellow:status\r",
          "transceiverId": 6
        },
        "13": {
          "id": 13,
          "bluePath": "/sys/class/leds/port7_led1:blue:status",
          "yellowPath": "/sys/class/leds/port7_led1:yellow:status\r",
          "transceiverId": 7
        },
        "14": {
          "id": 14,
          "bluePath": "/sys/class/leds/port7_led2:blue:status",
          "yellowPath": "/sys/class/leds/port7_led2:yellow:status\r",
          "transceiverId": 7
        },
        "15": {
          "id": 15,
          "bluePath": "/sys/class/leds/port8_led1:blue:status",
          "yellowPath": "/sys/class/leds/port8_led1:yellow:status\r",
          "transceiverId": 8
        },
        "16": {
          "id": 16,
          "bluePath": "/sys/class/leds/port8_led2:blue:status",
          "yellowPath": "/sys/class/leds/port8_led2:yellow:status\r",
          "transceiverId": 8
        },
        "17": {
          "id": 17,
          "bluePath": "/sys/class/leds/port9_led1:blue:status",
          "yellowPath": "/sys/class/leds/port9_led1:yellow:status\r",
          "transceiverId": 9
        }
      }
    }
  }
}
)";

static BspPlatformMappingThrift buildTahansb800bcPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Tahansb800bcBspPlatformMapping::Tahansb800bcBspPlatformMapping()
    : BspPlatformMapping(
          buildTahansb800bcPlatformMapping(kJsonBspPlatformMappingStr)) {}

Tahansb800bcBspPlatformMapping::Tahansb800bcBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildTahansb800bcPlatformMapping(platformMappingStr)) {
}

} // namespace fboss
} // namespace facebook
