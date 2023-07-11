// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <string>

namespace facebook::fboss::platform {

std::string getMokujinFSConfig() {
  return R"({
  "bsp" : "mokujin",
  "watchdog" : {
    "access" : {
      "accessType" : 0,
      "path" : "/sys/bus/i2c/drivers/iob_cpld/watchdog"
    },
    "value" : 1
  },
  "boost_on_dead_fan" : true,
  "boost_on_dead_sensor" : false,
  "boost_on_no_qsfp_after" : 90,
  "pwm_boost_value" : 70,
  "pwm_percent_lower_limit" : 10,
  "pwm_percent_upper_limit" : 70,
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
          "40" : 35,
          "42" : 50,
          "44" : 75,
          "50" : 100
        },
        "1" : {
          "42" : 35,
          "44" : 50,
          "46" : 75,
          "52" : 100
        },
        "2" : {
          "46" : 35,
          "48" : 50,
          "50" : 75,
          "52" : 100
        }
      }
    }
  ],
  "sensors" : {
    "test_sensor_1" : {
      "access" : {
        "accessType" : 2
      },
      "adjustment" : [
        [10,5],
        [25,4],
        [30,1]
      ],
      "type" : "linear_four_curves",
      "normal_up_table" : [
        [28.99, 30],
        [29, 35],
        [30, 40],
        [31, 45],
        [33, 50],
        [34, 55],
        [35, 60]
      ],
      "normal_down_table" : [
        [26.99, 30],
        [27, 35],
        [28, 40],
        [29, 45],
        [30, 50],
        [31, 55],
        [33, 60]
      ],
      "onefail_up_table" : [
        [27.99, 40],
        [28, 45],
        [29, 50],
        [31, 55],
        [33, 60],
        [34, 65],
        [35, 70]
      ],
      "onefail_down_table" : [
        [25.99, 40],
        [26, 45],
        [27, 50],
        [29, 55],
        [31, 60],
        [32, 65],
        [33, 70]
      ],
      "alarm" : {
        "highMajor" : 70.0,
        "highMinor" : 65.0,
        "minorSoakSeconds" : 15
      }
    },
    "test_sensor_2" : {
      "access" : {
        "accessType" : 2
      },
      "adjustment" : [
        [10,5],
        [25,4],
        [30,1]
      ],
      "type" : "linear_four_curves",
      "normal_up_table" : [
        [28.99, 30],
        [29, 35],
        [30, 40],
        [31, 45],
        [33, 50],
        [34, 55],
        [35, 60]
      ],
      "normal_down_table" : [
        [26.99, 30],
        [27, 35],
        [28, 40],
        [29, 45],
        [30, 50],
        [31, 55],
        [33, 60]
      ],
      "onefail_up_table" : [
        [27.99, 40],
        [28, 45],
        [29, 50],
        [31, 55],
        [33, 60],
        [34, 65],
        [35, 70]
      ],
      "onefail_down_table" : [
        [25.99, 40],
        [26, 45],
        [27, 50],
        [29, 55],
        [31, 60],
        [32, 65],
        [33, 70]
      ],
      "alarm" : {
        "highMajor" : 70.0,
        "highMinor" : 65.0,
        "minorSoakSeconds" : 15
      }
    },
    "test_sensor_3" : {
      "access" : {
        "accessType" : 1,
        "path" : "sensor-util --read --name:test_sensor_2"
      },
      "adjustment" : [
        [60,0],
        [80,0],
        [95,0]
      ],
      "type" : "pid",
      "setpoint" : 92,
      "positive_hysteresis" : 0,
      "negative_hysteresis" : 3,
      "kp" : 2,
      "ki" : 0.1,
      "kd" : 0.1,
      "alarm" : {
        "highMajor" : 120.0,
        "highMinor" : 105.0,
        "minorSoakSeconds" : 30
      },
      "range_check" : {
        "range_low" : 0,
        "range_high" : 130,
        "invalid_range_action" : "shutdown",
        "tolerance" : 10
      }
    },
    "test_sensor_4" : {
      "access" : {
        "accessType" : 1,
        "path" : "sensor-util --read --name:test_sensor_2"
      },
      "adjustment" : [
        [60,0],
        [80,0],
        [95,0]
      ],
      "type" : "incrementpid",
      "setpoint" : 92,
      "positive_hysteresis" : 0,
      "negative_hysteresis" : 3,
      "kp" : 2,
      "ki" : 0.5,
      "kd" : 0.5,
      "alarm" : {
        "highMajor" : 120.0,
        "highMinor" : 105.0,
        "minorSoakSeconds" : 30
      },
      "range_check" : {
        "range_low" : 0,
        "range_high" : 130,
        "invalid_range_action" : "shutdown",
        "tolerance" : 10
      }
    }
  },
  "fans" : {
    "fan_1" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan1_rpm"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan1_pwm"
      },
      "presence" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan1_presence"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan1_led"
      },
      "fan_good_led_val" : 1,
      "fan_fail_led_val" : 0
    },
    "fan_2" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan2_rpm"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan2_pwm"
      },
      "presence" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan2_presence"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan2_led"
      },
      "fan_good_led_val" : 1,
      "fan_fail_led_val" : 0
    },
    "fan_3" : {
      "rpm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan3_rpm"
      },
      "pwm" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan3_pwm"
      },
      "presence" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan3_presence"
      },
      "fan_present_val" : 1,
      "fan_missing_val" : 0,
      "led" : {
        "accessType" : 0,
        "path" : "/sys/bus/i2c/drivers/fan_cpld/fan3_led"
      },
      "fan_good_led_val" : 1,
      "fan_fail_led_val" : 0
    }
  },
  "zones" : [
    {
      "zoneType" : 0,
      "zoneName" : "left_zone",
      "sensorNames" : [
        "test_sensor_1",
        "test_sensor_2",
        "qsfp_group_1"
      ],
      "fanNames" : [
        "fan_1",
        "qsfp_group_1"
      ],
      "slope" : 3
    },
    {
      "zoneType" : 2,
      "zoneName" : "center_zone",
      "sensorNames" : [
        "test_sensor_3",
        "qsfp_group_1"
      ],
      "fanNames" : [
        "fan_2"
      ],
      "slope" : 3
    },
    {
      "zoneType" : 1,
      "zoneName" : "right_zone",
      "sensorNames" : [
        "test_sensor_4"
      ],
      "fanNames" : [
        "fan_3"
      ],
      "slope" : 3
    }
  ]
})";
}
} // namespace facebook::fboss::platform
