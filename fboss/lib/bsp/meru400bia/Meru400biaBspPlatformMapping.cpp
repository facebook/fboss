// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.h"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp1_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp1_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-1",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH0"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp2_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp2_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-2",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH1"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp3_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp3_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-3",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH2"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp4_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp4_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-4",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH3"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp5_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp5_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-5",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH4"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp6_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp6_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-6",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH5"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp7_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp7_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-7",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH6"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp8_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp8_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-8",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS0_CH7"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp9_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp9_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-9",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH0"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp10_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp10_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-10",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH1"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp11_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp11_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-11",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH2"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp12_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp12_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-12",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH3"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp13_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp13_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-13",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH4"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp14_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp14_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-14",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH5"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp15_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp15_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-15",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH6"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp16_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp16_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-16",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS1_CH7"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp17_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp17_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-17",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH0"
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
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp18_reset",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "presence": {
                  "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/scd-xcvr.0/qsfp18_present",
                  "mask": 1,
                  "gpioOffset": 0
                },
                "gpioChip": ""
              },
              "io": {
                "controllerId": "ioController-18",
                "type": 1,
                "devicePath": "/run/devmap/i2c-busses/SCD_SMBUS2_CH1"
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

static BspPlatformMappingThrift buildMeru400biaPlatformMapping() {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(kJsonBspPlatformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

// TODO: Use pre generated bsp platform mapping from cfgr
Meru400biaBspPlatformMapping::Meru400biaBspPlatformMapping()
    : BspPlatformMapping(buildMeru400biaPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
