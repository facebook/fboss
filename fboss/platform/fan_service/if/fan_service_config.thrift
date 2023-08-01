namespace cpp2 facebook.fboss.platform.fan_service

const string RANGE_CHECK_ACTION_SHUTDOWN = "shutdown";
const string ACCESS_TYPE_SYSFS = "ACCESS_TYPE_SYSFS";
const string ACCESS_TYPE_UTIL = "ACCESS_TYPE_UTIL";
const string ACCESS_TYPE_THRIFT = "ACCESS_TYPE_THRIFT";
const string ACCESS_TYPE_REST = "ACCESS_TYPE_REST";
const string ACCESS_TYPE_QSFP = "ACCESS_TYPE_QSFP";
const string ZONE_TYPE_MAX = "ZONE_TYPE_MAX";
const string ZONE_TYPE_MIN = "ZONE_TYPE_MIN";
const string ZONE_TYPE_AVG = "ZONE_TYPE_AVG";
const string OPTIC_AGGREGATION_TYPE_MAX = "OPTIC_AGGREGATION_TYPE_MAX";
const string SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE = "SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE";
const string SENSOR_PWM_CALC_TYPE_INCREMENT_PID = "SENSOR_PWM_CALC_TYPE_INCREMENT_PID";
const string SENSOR_PWM_CALC_TYPE_PID = "SENSOR_PWM_CALC_TYPE_PID";

enum OpticTableType {
  kOpticTable100Generic = 0,
  kOpticTable200Generic = 1,
  kOpticTable400Generic = 2,
  kOpticTable800Generic = 3,
  kOpticTableInval = 4,
}

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
typedef map<i32, float> TempToPwmMap

struct Optic {
  1: string opticName;
  2: AccessMethod access;
  3: list<i32> portList;
  4: string aggregationType;
  5: map<OpticTableType, TempToPwmMap> tempToPwmMaps;
}

struct Alarm {
  1: float highMajor;
  2: float highMinor;
  3: i32 minorSoakSeconds;
}

struct Fan {
  1: string fanName;
  2: AccessMethod rpmAccess;
  3: AccessMethod pwmAccess;
  4: AccessMethod presenceAccess;
  5: AccessMethod ledAccess;
  6: i32 pwmMin;
  7: i32 pwmMax;
  8: i32 fanPresentVal;
  9: i32 fanMissingVal;
  10: i32 fanGoodLedVal;
  11: i32 fanFailLedVal;
}

struct Watchdog {
  1: AccessMethod access;
  2: i32 value;
}

struct RangeCheck {
  1: float low;
  2: float high;
  3: i32 tolerance;
  4: string invalidRangeAction;
}

enum BspType {
  kBspGeneric = 0,
  kBspDarwin = 1,
  kBspLassen = 2,
  kBspMinipack3 = 3,
  kBspMokujin = 4,
}

struct Sensor {
  1: string sensorName;
  2: AccessMethod access;
  3: map<i32, i32> adjustmentTable;
  4: Alarm alarm;
  5: optional RangeCheck rangeCheck;
  6: string pwmCalcType;
  7: float scale;
  8: TempToPwmMap normalUpTable;
  9: TempToPwmMap normalDownTable;
  10: TempToPwmMap failUpTable;
  11: TempToPwmMap failDownTable;
  12: float setPoint;
  13: float posHysteresis;
  14: float negHysteresis;
  15: float kp;
  16: float kd;
  17: float ki;
}

struct FanServiceConfig {
  1: BspType bspType;
  2: string shutdownCmd;
  3: list<Zone> zones;
  4: list<Sensor> sensors;
  5: list<Optic> optics;
  6: list<Fan> fans;
  7: optional Watchdog watchdog;
  11: i16 pwmBoostOnNumDeadFan;
  12: i16 pwmBoostOnNumDeadSensor;
  13: i16 pwmBoostOnNoQsfpAfterInSec;
  14: i32 pwmBoostValue;
  15: i32 pwmTransitionValue;
  16: i32 pwmUpperThreshold;
  17: i32 pwmLowerThreshold;
}
