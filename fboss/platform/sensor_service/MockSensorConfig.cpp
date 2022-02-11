// Copyright 2004-present Facebook. All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::sensor_service {

std::string getMockConfig() {
  return R"({
  "source" : "mock",
  "sensorMapList" : {
    "SCM" : {
      "CPU_PHYS_ID_0" : {
        "path" : "coretemp-isa-0000:Package id 0",
        "maxVal" : 105,
        "unit"  : "C"
      },
      "CPU_CORE0_TEMP" : {
        "path" : "coretemp-isa-0000:Core 0",
        "maxVal" : 105,
        "unit" : "C"
      },
      "CPU_CORE1_TEMP" : {
        "path" : "coretemp-isa-0000:Core 0",
        "maxVal" : 105,
        "unit" : "C"
      }
    },

    "FAN1" : {
      "FAN1_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan1",
        "maxVal" : 25500,
        "minVal" : 2600,
        "unit" : "RPM"
      }
    },
    "FAN2" : {
      "FAN2_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan2",
        "maxVal" : 25500,
        "minVal" : 2600,
        "unit" : "RPM"
      }
    },
    "FAN3" : {
      "FAN3_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan3",
        "maxVal" : 25500,
        "minVal" : 2600,
        "unit" : "RPM"
      }
    },
    "FAN4" : {
      "FAN4_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan4",
        "maxVal" : 25500,
        "minVal" : 2600,
        "unit" : "RPM"
      }
    },
    "FAN5" : {
      "FAN5_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan5",
        "maxVal" : 25500,
        "minVal" : 2600,
        "unit" : "RPM"
      }
    }
  }
})";
}
} // namespace facebook::fboss::platform::sensor_service
