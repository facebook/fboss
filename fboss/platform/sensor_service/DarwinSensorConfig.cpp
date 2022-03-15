// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::sensor_service {

/* ToDo: replace hardcoded sysfs path with dynamically mapped symbolic path */

std::string getDarwinConfig() {
  return R"({
  "source" : "sysfs",
  "sensorMapList" : {
    "SCM" : {
      "PCH_TEMP" :{
        "path" : "/sys/class/hwmon/hwmon0/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "CPU_PHYS_ID_0" : {
        "path" : "/sys/class/hwmon/hwmon2/temp1_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type"  : 3
      },
      "CPU_CORE0_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon2/temp2_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      },
      "CPU_CORE1_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon2/temp3_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      }
    },

    "FAN1" : {
      "FAN1_RPM" : {
        "path" : "/sys/class/hwmon/hwmon3/fan1_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
        }
    },
    "FAN2" : {
      "FAN2_RPM" : {
        "path" : "/sys/class/hwmon/hwmon3/fan2_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },
    "FAN3" : {
      "FAN3_RPM" : {
        "path" : "/sys/class/hwmon/hwmon3/fan3_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type"  : 4
      }
    },
    "FAN4" : {
      "FAN4_RPM" : {
        "path" : "/sys/class/hwmon/hwmon3/fan4_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },
    "FAN5" : {
      "FAN5_RPM" : {
        "path" : "/sys/class/hwmon/hwmon3/fan5_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },

    "CPU_CARD" : {
      "CPU_BOARD_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon4/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "BACK_PANEL_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon4/temp2_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "MPS1_VIN" : {
        "path" : "/sys/class/hwmon/hwmon5/in1_input",
        "thresholdMap" : {
          "4" : 14,
          "5" : 9
        },
        "type" : 1
      },
      "MPS1_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon5/temp1_input",
        "thresholdMap" : {
          "4" : 110
        },
        "type" : 3
      },
      "MPS1_POUT" : {
        "path" : "/sys/class/hwmon/hwmon5/power2_input",
        "type" : 0
      },
      "MPS1_IIN" : {
        "path" : "/sys/class/hwmon/hwmon5/curr1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 2
      },
      "MPS1_IOUT" : {
        "path" : "/sys/class/hwmon/hwmon5/curr2_input",
        "thresholdMap" : {
          "4" : 45
        },
        "type" : 2
      },
      "MPS2_VIN" : {
        "path" : "/sys/class/hwmon/hwmon6/in1_input",
        "thresholdMap" : {
          "4" : 14,
          "5" : 9
        },
        "type" : 1
      },
      "MPS2_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon6/temp1_input",
        "thresholdMap" : {
          "4" : 110
        },
        "type" : 3
      },
      "MPS2_POUT" : {
        "path" : "/sys/class/hwmon/hwmon6/power2_input",
        "type" : 0
      },
      "MPS2_IIN" : {
        "path" : "/sys/class/hwmon/hwmon6/curr1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 2
      },
      "MPS2_IOUT" : {
        "path" : "/sys/class/hwmon/hwmon6/curr2_input",
        "thresholdMap" : {
          "4" : 35
        },
        "type" : 2
      },
      "POS_1V7_VCCIN_VRRDY" : {
        "path" : "/sys/class/hwmon/hwmon7/in1_input",
        "thresholdMap" : {
          "4" : 1.875,
          "5" : 1.12
        },
        "type" : 1
      },
      "POS_0V6_VTT" : {
        "path" : "/sys/class/hwmon/hwmon7/in2_input",
        "thresholdMap" : {
          "4" : 0.69,
          "5" : 0.51
        },
        "type" : 1
      },
      "POS_1V2_VDDQ" : {
        "path" : "/sys/class/hwmon/hwmon7/in3_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_2V5_VPP" : {
        "path" : "/sys/class/hwmon/hwmon7/in4_input",
        "thresholdMap" : {
          "4" : 2.99,
          "5" : 2.21
        },
        "type" : 1
      },
      "POS_1V5_PCH" : {
        "path" : "/sys/class/hwmon/hwmon7/in5_input",
        "thresholdMap" : {
          "4" : 1.725,
          "5" : 1.27
        },
        "type" : 1
      },
      "POS_1V05_COM" : {
        "path" : "/sys/class/hwmon/hwmon7/in6_input",
        "thresholdMap" : {
          "4" : 1.208,
          "5" : 0.89
        },
        "type" : 1
      },
      "POS_1V3_KRHV" : {
        "path" : "/sys/class/hwmon/hwmon7/in7_input",
        "thresholdMap" : {
          "4" : 1.495,
          "5" : 1.1
        },
        "type" : 1
      },
      "POS_1V7_SCFUSE" : {
        "path" : "/sys/class/hwmon/hwmon7/in8_input",
        "thresholdMap" : {
          "4" : 1.955,
          "5" : 1.44
        },
        "type" : 1
      },
      "POS_3V3" : {
        "path" : "/sys/class/hwmon/hwmon7/in9_input",
        "thresholdMap" : {
          "4" : 3.795,
          "5" : 2.8
        },
        "type" : 1
      },
      "POS_5V0" : {
        "path" : "/sys/class/hwmon/hwmon7/in10_input",
        "thresholdMap" : {
          "4" : 5.75,
          "5" : 4.25
        },
        "type" : 1
      },
      "POS_1V2_ALW" : {
        "path" : "/sys/class/hwmon/hwmon7/in11_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_3V3_ALW" : {
        "path" : "/sys/class/hwmon/hwmon7/in12_input",
        "thresholdMap" : {
          "4" : 3.795,
          "5" : 2.8
        },
        "type" : 1
      },
      "POS_12V" : {
        "path" : "/sys/class/hwmon/hwmon7/in13_input",
        "thresholdMap" : {
          "4" : 13.8,
          "5" : 9.72
        },
        "type" : 1
      },
      "POS_1V2_LAN1" : {
        "path" : "/sys/class/hwmon/hwmon7/in14_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_1V2_LAN2" : {
        "path" : "/sys/class/hwmon/hwmon7/in15_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "FRONT_PANEL_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon8/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      }
    },

    "SWITCH_CARD" : {
      "SC_BOARD_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "SC_BOARD_MIDDLE_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp2_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_BOARD_LEFT_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp3_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_FRONT_PANEL_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp4_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_TH3_DIODE1_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp7_input",
        "thresholdMap" : {
          "4" : 125
        },
        "type" : 3
      },
      "SC_TH3_DIODE2_TEMP" : {
        "path" : "/sys/class/hwmon/hwmon9/temp8_input",
        "thresholdMap" : {
          "4" : 125
        },
        "type" : 3
      }
    }

  }
})";
}
} // namespace facebook::fboss::platform::sensor_service
