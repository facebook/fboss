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
          "path": "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/hwmon/hwmon2/fan1_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-2",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/hwmon/hwmon2/fan2_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-3",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/hwmon/hwmon2/fan3_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-4",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/hwmon/hwmon2/fan4_present"
        }
      ]
    },
    {
      "name": "DarwinFanModule-5",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/hwmon/hwmon2/fan5_present"
        }
      ]
    },
    {
      "name": "DarwinPemModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/pci/drivers/scd/0000:07:00.0/pem_present"
        }
      ]
    },
    {
      "name": "DarwinRackmonModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/sys/bus/pci/drivers/scd/0000:07:00.0/rackmon_present"
        }
      ]
    }
  ],
  "chassisAttributes": [
    {
      "name": "SystemLedRed",
      "path": "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/red:sys_status/brightness"
    },
    {
      "name" : "SystemLedGreen",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/green:sys_status/brightness"
    },
    {
      "name" : "FanLedRed",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/red:fan_status/brightness"
    },
    {
      "name" : "FanLedGreen",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/green:fan_status/brightness"
    },
    {
      "name" : "PemLedRed",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/red:pem_status/brightness"
    },
    {
      "name" : "PemLedGreen",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/green:pem_status/brightness"
    },
    {
      "name" : "RackmonLedRed",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/red:rackmon_status/brightness"
    },
    {
      "name" : "RackmonLedGreen",
      "path" : "/sys/bus/pci/drivers/scd/0000:07:00.0/scd-leds.5.auto/leds/green:rackmon_status/brightness"
    }
  ]
})";
}

} // namespace facebook::fboss::platform::data_corral_service
