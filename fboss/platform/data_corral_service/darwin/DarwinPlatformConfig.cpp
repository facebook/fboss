// Copyright 2014-present Facebook. All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::data_corral_service {

std::string getDarwinPlatformConfig() {
  // TODO: return based on the config file from FLAGS_config if necessary
  // For now, just hardcode darwin platform config
  return R"({
  "fruModules": [
    {
      "name": "DarwinFanModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/sensors/FAN_CPLD/fan1_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-2",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/sensors/FAN_CPLD/fan2_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-3",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/sensors/FAN_CPLD/fan3_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-4",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/sensors/FAN_CPLD/fan4_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-5",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/sensors/FAN_CPLD/fan5_present"
        }
      ]
    },
    {
      "name": "DarwinPemModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA/pem_present"
        }
      ]
    },
    {
      "name": "DarwinRackmonModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA/rackmon_present"
        }
      ]
    }
  ],
  "chassisAttributes": [
    {
      "name": "SystemLedRed",
      "path": "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/system:red:status/brightness"
    },
    {
      "name" : "SystemLedGreen",
      "path" : "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/system:green:status/brightness"
    },
    {
      "name" : "FanLedRed",
      "path" : "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/fan:red:status/brightness"
    },
    {
      "name" : "FanLedGreen",
      "path" : "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/fan:green:status/brightness"
    },
    {
      "name" : "PemLedRed",
      "path" : "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/pem:red:status/brightness"
    },
    {
      "name" : "PemLedGreen",
      "path" : "/run/devmap/fpgas/SCD_FPGA/scd-leds.0/leds/pem:green:status/brightness"
    }
  ]
})";
}

} // namespace facebook::fboss::platform::data_corral_service
