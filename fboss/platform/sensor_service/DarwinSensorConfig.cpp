// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include <string>

namespace facebook::fboss::platform::sensor_service {

/* ToDo: replace hardcoded sysfs path with dynamically mapped symbolic path */

std::string getDarwinConfig() {
  return R"({
  "source" : "sysfs",
  "sensorMapList" : {
    "CPU_CARD" : {
      "PCH_TEMP" :{
        "path" : "/sys/devices/virtual/thermal/thermal_zone0/temp",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "CPU_PHYS_ID_0" : {
        "path" : "/sys/devices/platform/coretemp.0/.+/temp1_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type"  : 3
      },
      "CPU_CORE0_TEMP" : {
        "path" : "/sys/devices/platform/coretemp.0/.+/temp2_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      },
      "CPU_CORE1_TEMP" : {
        "path" : "/sys/devices/platform/coretemp.0/.+/temp3_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      },
      "CPU_CORE2_TEMP" : {
        "path" : "/sys/devices/platform/coretemp.0/.+/temp4_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      },
      "CPU_CORE3_TEMP" : {
        "path" : "/sys/devices/platform/coretemp.0/.+/temp5_input",
        "thresholdMap" : {
          "4" : 105
        },
        "type" : 3
      },
      "CPU_BOARD_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/lm90/9-004c/.+/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "BACK_PANEL_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/lm90/9-004c/.+/temp2_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "MPS1_VIN" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0021/.+/in1_input",
        "thresholdMap" : {
          "4" : 14,
          "5" : 9
        },
        "type" : 1
      },
      "MPS1_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0021/.+/temp1_input",
        "thresholdMap" : {
          "4" : 110
        },
        "type" : 3
      },
      "MPS1_POUT" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0021/.+/power2_input",
        "type" : 0
      },
      "MPS1_IIN" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0021/.+/curr1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 2
      },
      "MPS1_IOUT" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0021/.+/curr2_input",
        "thresholdMap" : {
          "4" : 45
        },
        "type" : 2
      },
      "MPS2_VIN" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0027/.+/in1_input",
        "thresholdMap" : {
          "4" : 14,
          "5" : 9
        },
        "type" : 1
      },
      "MPS2_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0027/.+/temp1_input",
        "thresholdMap" : {
          "4" : 110
        },
        "type" : 3
      },
      "MPS2_POUT" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0027/.+/power2_input",
        "type" : 0
      },
      "MPS2_IIN" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0027/.+/curr1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 2
      },
      "MPS2_IOUT" : {
        "path" : "/sys/bus/i2c/drivers/pmbus/11-0027/.+/curr2_input",
        "thresholdMap" : {
          "4" : 35
        },
        "type" : 2
      },
      "POS_1V7_VCCIN_VRRDY" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in1_input",
        "thresholdMap" : {
          "4" : 1.875,
          "5" : 1.12
        },
        "type" : 1
      },
      "POS_0V6_VTT" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in2_input",
        "thresholdMap" : {
          "4" : 0.69,
          "5" : 0.51
        },
        "type" : 1
      },
      "POS_1V2_VDDQ" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in3_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_2V5_VPP" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in4_input",
        "thresholdMap" : {
          "4" : 2.99,
          "5" : 2.21
        },
        "type" : 1
      },
      "POS_1V5_PCH" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in5_input",
        "thresholdMap" : {
          "4" : 1.725,
          "5" : 1.27
        },
        "type" : 1
      },
      "POS_1V05_COM" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in6_input",
        "thresholdMap" : {
          "4" : 1.208,
          "5" : 0.89
        },
        "type" : 1
      },
      "POS_1V3_KRHV" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in7_input",
        "thresholdMap" : {
          "4" : 1.495,
          "5" : 1.1
        },
        "type" : 1
      },
      "POS_1V7_SCFUSE" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in8_input",
        "thresholdMap" : {
          "4" : 1.955,
          "5" : 1.44
        },
        "type" : 1
      },
      "POS_3V3" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in9_input",
        "thresholdMap" : {
          "4" : 3.795,
          "5" : 2.8
        },
        "type" : 1
      },
      "POS_5V0" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in10_input",
        "thresholdMap" : {
          "4" : 5.75,
          "5" : 4.25
        },
        "type" : 1
      },
      "POS_1V2_ALW" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in11_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_3V3_ALW" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in12_input",
        "thresholdMap" : {
          "4" : 3.795,
          "5" : 2.8
        },
        "type" : 1
      },
      "POS_12V" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in13_input",
        "thresholdMap" : {
          "4" : 13.8,
          "5" : 9.72
        },
        "type" : 1
      },
      "POS_1V2_LAN1" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in14_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "POS_1V2_LAN2" : {
        "path" : "/sys/bus/i2c/drivers/ucd9000/10-004e/.+/in15_input",
        "thresholdMap" : {
          "4" : 1.38,
          "5" : 1.02
        },
        "type" : 1
      },
      "FRONT_PANEL_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/lm73/19-0048/.+/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      }
    },

    "FAN1" : {
      "FAN1_RPM" : {
        "path" : "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/.+/fan1_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
        }
    },
    "FAN2" : {
      "FAN2_RPM" : {
        "path" : "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/.+/fan2_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },
    "FAN3" : {
      "FAN3_RPM" : {
        "path" : "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/.+/fan3_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type"  : 4
      }
    },
    "FAN4" : {
      "FAN4_RPM" : {
        "path" : "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/.+/fan4_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },
    "FAN5" : {
      "FAN5_RPM" : {
        "path" : "/sys/bus/i2c/drivers/rook-fan-cpld/17-0060/.+/fan5_input",
        "thresholdMap" : {
          "4" : 25500,
          "5" : 2600
        },
        "type" : 4
      }
    },

    "SWITCH_CARD" : {
      "SC_BOARD_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "SC_BOARD_MIDDLE_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp2_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_BOARD_LEFT_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp3_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_FRONT_PANEL_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp4_input",
        "thresholdMap" : {
          "4" : 75
        },
        "type" : 3
      },
      "SC_TH3_DIODE1_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp7_input",
        "thresholdMap" : {
          "4" : 125
        },
        "type" : 3
      },
      "SC_TH3_DIODE2_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/max6697/1-004d/.+/temp8_input",
        "thresholdMap" : {
          "4" : 125
        },
        "type" : 3
      }
    },

    "PEM" : {
      "PEM_ECB_VOUT_CH1" : {
        "path" : "/sys/bus/i2c/drivers/amax5970/4-003a/.+/in1_input",
        "thresholdMap" : {
          "4" : 14
        },
        "compute" : "15.5*@",
        "type" : 1
      },
      "PEM_ECB_VOUT_CH2" : {
        "path" : "/sys/bus/i2c/drivers/amax5970/4-003a/.+/in2_input",
        "thresholdMap" : {
          "4" : 14
        },
        "compute" : "15.5*@",
        "type" : 1
      },
      "PEM_ECB_IOUT_CH1" : {
        "path" : "/sys/bus/i2c/drivers/amax5970/4-003a/.+/curr1_input",
        "thresholdMap" : {
          "4" : 60,
          "5" : 0.5
        },
        "compute" : "(48390/343)*@",
        "type" : 2
      },
      "PEM_ECB_IOUT_CH2" : {
        "path" : "/sys/bus/i2c/drivers/amax5970/4-003a/.+/curr2_input",
        "thresholdMap" : {
          "4" : 60,
          "5" : 0.5
        },
        "compute" : "(48390/343)*@",
        "type" : 2
      },
      "PEM_ADC_VIN" : {
        "path" : "/sys/bus/i2c/drivers/max1363/4-0036/.+/in_voltage1_raw",
        "thresholdMap" : {
          "4" : 13.5,
          "5" : 10.9
        },
        "type" : 1
      },
      "PEM_ADC_VOUT" : {
        "path" : "/sys/bus/i2c/drivers/max1363/4-0036/.+/in_voltage0_raw",
        "thresholdMap" : {
          "5" : 10.8
        },
        "type" : 1
      },
      "PEM_ADC_VDROP" : {
        "path" : "/sys/bus/i2c/drivers/max1363/4-0036/.+/in_voltage1-voltage0_raw",
        "thresholdMap" : {
          "4" : 0.08,
          "5" : 0.0
        },
        "type" : 1
      },
      "PEM_INTERNAL_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/lm90/4-004c/.+/temp1_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      },
      "PEM_EXTERNAL_TEMP" : {
        "path" : "/sys/bus/i2c/drivers/lm90/4-004c/.+/temp2_input",
        "thresholdMap" : {
          "4" : 85
        },
        "type" : 3
      }
    },

    "FanSpiner" : {
      "FS_FAN_RPM" : {
        "path" : "/sys/bus/i2c/drivers/aslg4f4527/5-0008/.+/fan1_input",
        "thresholdMap" : {
          "4" : 29500,
          "5" : 2600
        },
        "type" : 4
      }
    }
  }
})";
}
} // namespace facebook::fboss::platform::sensor_service
