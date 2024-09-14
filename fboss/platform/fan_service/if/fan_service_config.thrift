namespace cpp2 facebook.fboss.platform.fan_service
namespace py3 neteng.fboss.platform.fan_service

const string ACCESS_TYPE_SYSFS = "ACCESS_TYPE_SYSFS";
const string ACCESS_TYPE_UTIL = "ACCESS_TYPE_UTIL";
const string ACCESS_TYPE_THRIFT = "ACCESS_TYPE_THRIFT";
const string ACCESS_TYPE_QSFP = "ACCESS_TYPE_QSFP";
const string ZONE_TYPE_MAX = "ZONE_TYPE_MAX";
const string ZONE_TYPE_MIN = "ZONE_TYPE_MIN";
const string ZONE_TYPE_AVG = "ZONE_TYPE_AVG";
const string OPTIC_TYPE_100_GENERIC = "OPTIC_TYPE_100_GENERIC";
const string OPTIC_TYPE_200_GENERIC = "OPTIC_TYPE_200_GENERIC";
const string OPTIC_TYPE_400_GENERIC = "OPTIC_TYPE_400_GENERIC";
const string OPTIC_TYPE_800_GENERIC = "OPTIC_TYPE_800_GENERIC";
const string OPTIC_AGGREGATION_TYPE_MAX = "OPTIC_AGGREGATION_TYPE_MAX";
const string OPTIC_AGGREGATION_TYPE_PID = "OPTIC_AGGREGATION_TYPE_PID";
const string SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE = "SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE";
const string SENSOR_PWM_CALC_TYPE_INCREMENT_PID = "SENSOR_PWM_CALC_TYPE_INCREMENT_PID";
const string SENSOR_PWM_CALC_TYPE_PID = "SENSOR_PWM_CALC_TYPE_PID";

struct AccessMethod {
  1: string accessType;
  2: string path;
}

struct Zone {
  1: string zoneType;
  2: string zoneName;
  3: list<string> sensorNames;
  4: list<string> fanNames;
  5: float slope;
}

# If the read temperature exceeds the specified temperature,
# set the PWM to the specified value.
typedef map<i32, i16> TempToPwmMap

# PID specific settings
# setPoint : Target set point, affecting ki based calculation
# kp : Proportional gain in PID method
# ki : Integral gain in PID method
# kd : Derivative gain in PID method
# negHysteresis : the maximum downward PID value change per control
# posHysteresis : the maximum upward PID value change per control
struct PidSetting {
  1: float kp;
  2: float ki;
  3: float kd;
  4: float setPoint;
  5: float negHysteresis;
  6: float posHysteresis;
}

struct Optic {
  1: string opticName;
  2: AccessMethod access;
  3: list<i32> portList;
  4: string aggregationType;
  5: map<string/* optic_type */ , TempToPwmMap> tempToPwmMaps;
  6: map<string/* optic_type */ , PidSetting> pidSettings;
}

struct Gpio {
  1: string path;
  2: i32 lineIndex;
  3: i16 desiredValue;
}

struct Fan {
  1: string fanName;
  2: string rpmSysfsPath;
  3: string pwmSysfsPath;
  4: optional string presenceSysfsPath;
  5: string ledSysfsPath;
  6: i32 pwmMin;
  7: i32 pwmMax;
  8: i32 fanPresentVal;
  9: i32 fanMissingVal;
  10: i32 fanGoodLedVal;
  11: i32 fanFailLedVal;
  12: optional Gpio presenceGpio;
  13: optional i32 rpmMin;
  14: optional i32 rpmMax;
}

struct Watchdog {
  1: AccessMethod access;
  2: i32 value;
}

struct Sensor {
  1: string sensorName;
  2: AccessMethod access;
  6: string pwmCalcType;
  7: float scale;
  8: TempToPwmMap normalUpTable;
  9: TempToPwmMap normalDownTable;
  10: TempToPwmMap failUpTable;
  11: TempToPwmMap failDownTable;
  12: PidSetting pidSetting;
}

struct ControlInterval {
  1: i32 sensorReadInterval;
  2: i32 pwmUpdateInterval;
}

struct FanServiceConfig {
  1: optional ControlInterval controlInterval;
  2: string shutdownCmd;
  3: list<Zone> zones;
  4: list<Sensor> sensors;
  5: list<Optic> optics;
  6: list<Fan> fans;
  7: optional Watchdog watchdog;
  11: i16 pwmBoostOnNumDeadFan;
  12: i16 pwmBoostOnNumDeadSensor;
  13: i16 pwmBoostOnNoQsfpAfterInSec;
  14: i16 pwmBoostValue;
  15: i16 pwmTransitionValue;
  16: i16 pwmUpperThreshold;
  17: i16 pwmLowerThreshold;
}
