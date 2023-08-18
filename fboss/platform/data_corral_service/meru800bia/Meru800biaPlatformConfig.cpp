// Copyright 2023-present Facebook. All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::data_corral_service {

std::string getMeru800biaPlatformConfig() {
  // TODO: return based on the config file from FLAGS_config if necessary
  // For now, just hardcode meru800bia platform config
  return R"({
  "fruModules": [
  ],
  "chassisAttributes": [
    {
      "name": "SystemLedAmber",
      "path": "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/system:amber:status/brightness"
    },
    {
      "name" : "SystemLedBlue",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/system:blue:status/brightness"
    },
    {
      "name" : "FanLedAmber",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/fan:amber:status/brightness"
    },
    {
      "name" : "FanLedBlue",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/fan:blue:status/brightness"
    },
    {
      "name" : "PsuLedAmber",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/psu:amber:status/brightness"
    },
    {
      "name" : "PsuLedBlue",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/psu:blue:status/brightness"
    },
    {
      "name" : "SmbLedAmber",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/switch_module:amber:status/brightness"
    },
    {
      "name" : "SmbLedBlue",
      "path" : "/run/devmap/fpgas/MERU_SCM_CPLD/scd-leds.0/leds/switch_module:blue:status/brightness"
    }
  ]
})";
}

} // namespace facebook::fboss::platform::data_corral_service
