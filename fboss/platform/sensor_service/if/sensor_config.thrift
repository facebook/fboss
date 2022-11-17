namespace cpp2 facebook.fboss.platform.sensor_config
namespace go neteng.fboss.platform.sensor_config
namespace py neteng.fboss.platform.sensor_config
namespace py3 neteng.fboss.platform.sensor_config
namespace py.asyncio neteng.fboss.platform.asyncio.sensor_config

enum SensorType {
  POWER = 0,
  VOLTAGE = 1,
  CURRENT = 2,
  TEMPERTURE = 3,
  FAN = 4,
  PWM = 5,
}

enum ThresholdType {
  MAX = 0,
  MIN = 1,
  MAX_ALARM = 2,
  MIN_ALARM = 3,
  UPPER_CRIT = 4,
  LOWER_CRIT = 5,
}

// Threshold value should be actual human readable value, e.g.
//  For Power, shold be Watt (W)
//  For Voltage, should be Volt (V)
//  For Current, should be Ampere (A)
//  For Temperature, should be celsius (C)
//  For Fan, should be RPM
//  For PWM, should be Percent(%)
typedef double ThresholdValue
typedef map<ThresholdType, ThresholdValue> ThresholdMap

struct Sensor {
  // Either sysfs path, symbolic or the path in "lmsensor" JSON output
  1: string path;
  // threshold list, contains the manufacture provided threshold values
  2: ThresholdMap thresholdMap = {};
  // Compute method, same format and calculation approch as lm_sensor, e.g. @*0.1
  3: optional string compute;
  4: SensorType type;
}

typedef string SensorName
typedef map<SensorName, Sensor> sensorMap
typedef string FruName

// The configuration for sensor mapping.  Each platform should provide one from
// vendor or derive from vendor spec sheet
struct SensorConfig {
  // source can be "lmsensor", "sysfs", or "mock"
  1: string source;
  2: map<FruName, sensorMap> sensorMapList;
}
