// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <string>

namespace facebook::fboss::platform::fbdevd {

std::string getDarwinDeviceTopology() {
  return R"({
    "platformName": "Darwin",
    "kmods": [
      "scd",
      "scd-smbus",
    ],
    "bootstrapOps": {
      "actions": []
    },
    "i2cClients": [
      {
        "bus": "ROOK_SMBUS3_CH0",
        "addr": 96,
        "deviceName": "tehama_cpld"
      },
      {
        "bus": "ROOK_SMBUS2_CH0",
        "addr": 35,
        "deviceName": "blackhawk_cpld"
      },
      {
        "bus": "SCD_SMBUS1_CH0",
        "addr": 77,
        "deviceName": "max6581"
      },
      {
        "bus": "ROOK_SMBUS0_CH0",
        "addr": 76,
        "deviceName": "max6658"
      },
      {
        "bus": "ROOK_SMBUS0_CH1",
        "addr": 78,
        "deviceName": "ucd90160"
      },
      {
        "bus": "ROOK_SMBUS0_CH2",
        "addr": 33,
        "deviceName": "pmbus"
      },
      {
        "bus": "ROOK_SMBUS2_CH2",
        "addr": 17,
        "deviceName": "ucd90320"
      },
      {
        "bus": "ROOK_SMBUS3_CH2",
        "addr": 72,
        "deviceName": "lm73"
      }
    ],
    "fruList": [
      {
        "fruName": "PEM",
        "fruState": {
          "condType": 2,
          "device": {
            "sysfsHandle": {
              "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/pem_present",
              "desiredValue": "0x1"
            }
          }
        },
        "initOps": {
          "actions": []
        },
        "cleanupOps": [],
        "i2cClients": [
          {
            "bus": "SCD_SMBUS1_CH3",
            "addr": 54,
            "deviceName": "max11645"
          },
          {
            "bus": "SCD_SMBUS1_CH3",
            "addr": 58,
            "deviceName": "amax5970"
          },
          {
            "bus": "SCD_SMBUS1_CH3",
            "addr": 76,
            "deviceName": "max6658"
          },
          {
            "bus": "SCD_SMBUS1_CH3",
            "addr": 80,
            "deviceName": "24c512"
          }
        ],
        "childFruList": []
      },
      {
        "fruName": "FanSpinner",
        "fruState": {
          "condType": 2,
          "device": {
            "sysfsHandle": {
              "sysfsPath": "/run/devmap/fpgas/SCD_FPGA/rackmon_present",
              "desiredValue": "0x1"
            }
          }
        },
        "initOps": {
          "actions": []
        },
        "cleanupOps": [],
        "i2cClients": [
          {
            "bus": "SCD_SMBUS1_CH4",
            "addr": 8,
            "deviceName": "aslg4f4527"
          },
          {
            "bus": "SCD_SMBUS1_CH4",
            "addr": 80,
            "deviceName": "24c512"
          },
          {
            "bus": "SCD_SMBUS1_CH4",
            "addr": 82,
            "deviceName": "24c512"
          },
          {
            "bus": "SCD_SMBUS1_CH4",
            "addr": 116,
            "deviceName": "pca9539"
          }
        ],
        "childFruList": []
      }
    ]
  })";
}

} // namespace facebook::fboss::platform::fbdevd
