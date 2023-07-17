// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <string>

namespace facebook::fboss::platform {

/* ToDo: replace hardcoded sysfs path with dynamically mapped symbolic path */

std::string getDarwinFSConfig() {
  return R"({
  "bsp" : "darwin",
  "boost_on_dead_fan" : true,
  "boost_on_dead_sensor" : false,
  "boost_on_no_qsfp_after" : 90,
  "pwm_boost_value" : 60,
  "pwm_transition_value" : 50,
  "pwm_percent_lower_limit" : 24,
  "pwm_percent_upper_limit" : 100,
  "shutdown_command" : "wedge_power reset -s",
  "optics" : [
    {
      "opticName" : "qsfp_group_1",
      "access" : {
        "accessType" : 5
      },
      "portList" : [],
      "aggregationType" : 0,
      "tempToPwmMaps" : {
        "0" : {
          "5" : 24,
          "38" : 26,
          "40" : 28,
          "41" : 30,
          "42" : 32,
          "44" : 34,
          "45" : 36,
          "48" : 38,
          "49" : 40,
          "52" : 44,
          "53" : 46,
          "54" : 50
        },
        "1" : {
          "5" : 26,
          "43" : 28,
          "45" : 30,
          "47" : 32,
          "49" : 34,
          "50" : 36,
          "54" : 40,
          "56" : 44,
          "58" : 46,
          "61" : 50
        },
        "2" : {
          "5" : 36,
          "59" : 40,
          "62" : 42,
          "66" : 46,
          "67" : 48,
          "68" : 50,
          "71" : 52,
          "73" : 55,
          "74" : 60
        }
      }
    }
  ],
  "sensors" : [
    {
      "sensorName" : "SC_TH3_DIODE1_TEMP",
      "access" : {
        "accessType" : 2
      },
      "alarm" : {
        "highMajor" : 105.0,
        "highMinor" : 90.0,
        "minorSoakSeconds" : 15
      },
      "calcType" : 0,
      "scale" : 1000.0,
      "normalUpTable" : {
        "15" : 24,
        "110" : 100
      },
      "normalDownTable" : {
        "15" : 24,
        "110" : 100
      },
      "failUpTable" : {
        "15" : 24,
        "110" : 100
      },
      "failDownTable" : {
        "15" : 24,
        "110" : 100
      }
    },
    {
      "sensorName" : "SC_TH3_DIODE2_TEMP",
      "access" : {
        "accessType" : 2
      },
      "alarm" : {
        "highMajor" : 105.0,
        "highMinor" : 90.0,
        "minorSoakSeconds" : 15
      },
      "calcType" : 0,
      "scale" : 1000.0,
      "type" : "linear_four_curves",
      "normalUpTable" : {
        "15" : 24,
        "110" : 100
      },
      "normalDownTable" : {
        "15" : 24,
        "110" : 100
      },
      "failUpTable" : {
        "15" : 24,
        "110" : 100
      },
      "failDownTable" : {
        "15" : 24,
        "110" : 100
      }
    }
  ],
  "fans" : [
    {
      "fanName" : "fan_1",
      "rpmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_input"
      },
      "pwmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm1"
      },
      "presenceAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_present"
      },
      "ledAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_led"
      },
      "pwmMin" : 1,
      "pwmMax" : 255,
      "fanPresentVal" : 1,
      "fanMissingVal" : 0,
      "fanGoodLedVal" : 2,
      "fanFailLedVal" : 1
    },
    {
      "fanName" : "fan_2",
      "rpmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_input"
      },
      "pwmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm2"
      },
      "presenceAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_present"
      },
      "ledAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_led"
      },
      "pwmMin" : 1,
      "pwmMax" : 255,
      "fanPresentVal" : 1,
      "fanMissingVal" : 0,
      "fanGoodLedVal" : 2,
      "fanFailLedVal" : 1
    },
    {
      "fanName" : "fan_3",
      "rpmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_input"
      },
      "pwmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm3"
      },
      "presenceAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_present"
      },
      "ledAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_led"
      },
      "pwmMin" : 1,
      "pwmMax" : 255,
      "fanPresentVal" : 1,
      "fanMissingVal" : 0,
      "fanGoodLedVal" : 2,
      "fanFailLedVal" : 1
    },
    {
      "fanName" : "fan_4",
      "rpmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_input"
      },
      "pwmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm4"
      },
      "presenceAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_present"
      },
      "ledAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_led"
      },
      "pwmMin" : 1,
      "pwmMax" : 255,
      "fanPresentVal" : 1,
      "fanMissingVal" : 0,
      "fanGoodLedVal" : 2,
      "fanFailLedVal" : 1
    },
    {
      "fanName" : "fan_5",
      "rpmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_input"
      },
      "pwmAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm5"
      },
      "presenceAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_present"
      },
      "ledAccess" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_led"
      },
      "pwmMin" : 1,
      "pwmMax" : 255,
      "fanPresentVal" : 1,
      "fanMissingVal" : 0,
      "fanGoodLedVal" : 2,
      "fanFailLedVal" : 1

    }
  ],
  "zones": [
    {
      "zoneType" : 0,
      "zoneName" : "zone1",
      "sensorNames" : [
        "SC_TH3_DIODE1_TEMP",
        "SC_TH3_DIODE2_TEMP",
        "qsfp_group_1"
      ],
      "fanNames" : [
        "fan_1",
        "fan_2",
        "fan_3",
        "fan_4",
        "fan_5",
        "fan_6"
      ],
      "slope" : 3
    }
  ]
})";
}
} // namespace facebook::fboss::platform
