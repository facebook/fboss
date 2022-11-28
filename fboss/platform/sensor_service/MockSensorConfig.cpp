// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::sensor_service {

std::string getMockConfig() {
  return R"({
  "source" : "mock",
  "sensorMapList" : {
    "SCM" : {
      "CPU_PHYS_ID_0" : {
        "path" : "coretemp-isa-0000:Package id 0",
        "thresholds" : {
          "lowerCriticalVal" : 105
        },
        "compute" : "@*1",
        "type"  : 1
      },
      "CPU_CORE0_TEMP" : {
        "path" : "coretemp-isa-0000:Core 0",
        "thresholds" : {
           "lowerCriticalVal" : 105
        },
        "compute" : "@*1",
        "type" : 3
      },
      "CPU_CORE1_TEMP" : {
        "path" : "coretemp-isa-0000:Core 0",
        "thresholds" : {
           "lowerCriticalVal" : 105
        },
        "compute" : "@*1",
        "type" : 3
      }
    },

    "FAN1" : {
      "FAN1_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan1",
        "thresholds" : {
          "upperCriticalVal" : 25500,
          "lowerCriticalVal" : 2600
        },
        "compute" : "@*1",
        "type" : 4
      }
    },
    "FAN2" : {
      "FAN2_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan2",
        "thresholds" : {
          "upperCriticalVal" : 25500,
          "lowerCriticalVal" : 2600
        },
        "compute" : "@*1",
        "type" : 4
      }
    },
    "FAN3" : {
      "FAN3_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan3",
        "thresholds" : {
          "upperCriticalVal" : 25500,
          "lowerCriticalVal" : 2600
        },
        "compute" : "@*1",
        "type" : 4
      }
    },
    "FAN4" : {
      "FAN4_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan4",
        "thresholds" : {
          "upperCriticalVal" : 25500,
          "lowerCriticalVal" : 2600
        },
        "compute" : "@*1",
        "type" : 4
      }
    },
    "FAN5" : {
      "FAN5_RPM" : {
        "path" : "tehama_cpld-i2c-17-60:fan5",
        "thresholds" : {
          "upperCriticalVal" : 25500,
          "lowerCriticalVal" : 2600
        },
        "compute" : "@*1",
        "type" : 4
      }
    }
  }
})";
}
} // namespace facebook::fboss::platform::sensor_service
