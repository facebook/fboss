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
  "sensors" : {
    "SC_TH3_DIODE1_TEMP" : {
      "scale" : 1000.0,
      "access" : {
        "accessType" : 2
      },
      "adjustment" : [
        [0,0]
      ],
      "type" : "linear_four_curves",
      "normal_up_table" : [
        [15, 24],
        [110, 100]
      ],
      "normal_down_table" : [
        [15, 24],
        [110, 100]
      ],
      "onefail_up_table" : [
        [15, 24],
        [110, 100]
      ],
      "onefail_down_table" : [
        [15, 24],
        [110, 100]
      ],
      "alarm" : {
        "alarm_major" : 105.0,
        "alarm_minor" : 90.0,
        "alarm_minor_soak" : 15
      }
    },
    "SC_TH3_DIODE2_TEMP" : {
      "scale" : 1000.0,
      "access" : {
        "accessType" : 2
      },
      "adjustment" : [
        [0,0]
      ],
      "type" : "linear_four_curves",
      "normal_up_table" : [
        [15, 24],
        [110, 100]
      ],
      "normal_down_table" : [
        [15, 24],
        [110, 100]
      ],
      "onefail_up_table" : [
        [15, 24],
        [110, 100]
      ],
      "onefail_down_table" : [
        [15, 24],
        [110, 100]
      ],
      "alarm" : {
        "alarm_major" : 105.0,
        "alarm_minor" : 90.0,
        "alarm_minor_soak" : 15
      }
    }
  },
  "fans" : {
    "fan_1" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_input"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm1"
      },
      "pwm_range_min" : 1,
      "pwm_range_max" : 255,
      "presence" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_present"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan1_led"
      },
      "fan_good_led_val" : 2,
      "fan_fail_led_val" : 1
    },
    "fan_2" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_input"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm2"
      },
      "pwm_range_min" : 1,
      "pwm_range_max" : 255,
      "presence" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_present"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan2_led"
      },
      "fan_good_led_val" : 2,
      "fan_fail_led_val" : 1
    },
    "fan_3" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_input"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm3"
      },
      "pwm_range_min" : 1,
      "pwm_range_max" : 255,
      "presence" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_present"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan3_led"
      },
      "fan_good_led_val" : 2,
      "fan_fail_led_val" : 1
    },
    "fan_4" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_input"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm4"
      },
      "pwm_range_min" : 1,
      "pwm_range_max" : 255,
      "presence" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_present"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan4_led"
      },
      "fan_good_led_val" : 2,
      "fan_fail_led_val" : 1
    },
    "fan_5" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_input"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/pwm5"
      },
      "pwm_range_min" : 1,
      "pwm_range_max" : 255,
      "presence" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_present"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/run/devmap/sensors/FAN_CPLD/fan5_led"
      },
      "fan_good_led_val" : 2,
      "fan_fail_led_val" : 1
    }
  },
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
