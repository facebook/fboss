namespace cpp2 facebook.fboss.platform.sensor_config
namespace go neteng.fboss.platform.sensor_config
namespace py neteng.fboss.platform.sensor_config
namespace py3 neteng.fboss.platform.sensor_config
namespace py.asyncio neteng.fboss.platform.asyncio.sensor_config

// `SensorType` :  SensorType represents the type of sensor being measured.
//  For Power, should be Watt (W)
//  For Voltage, should be Volt (V)
//  For Current, should be Ampere (A)
//  For Temperature, should be celsius (C)
//  For Fan, should be RPM
//  For PWM, should be Percent(%)
enum SensorType {
  POWER = 0,
  VOLTAGE = 1,
  CURRENT = 2,
  TEMPERTURE = 3,
  FAN = 4,
  PWM = 5,
}

// `Threshold`: defined the various threshold below for various sensors
//
// `maxAlarmVal`: Threshold where if sensor value goes above it,
//                then an alarm should be triggered
//
// `minAlarmVal`: Threshold where if sensor value goes below it,
//                then an alarm should be triggered
//
// `upperCriticalVal`: Threshold where if sensor value goes above it, then both
//                     an alarm and self-healing remediation logic should kick
//                     in
//
// `lowerCriticalVal`: Threshold where if sensor value goes below it, then both
//                     an alarm and self-healing remediation logic should kick
//                     in
struct Thresholds {
  3: optional double maxAlarmVal;
  4: optional double minAlarmVal;
  5: optional double upperCriticalVal;
  6: optional double lowerCriticalVal;
}

struct Sensor {
  // Sysfs path
  1: string path;
  // Contains the manufacture provided threshold values
  2: optional Thresholds thresholds;
  // Compute method, same format and calculation approach as lm_sensor, e.g. @*0.1
  3: optional string compute;
  // contain various sensor type
  4: SensorType type;
}

typedef string SensorName
typedef map<SensorName, Sensor> sensorMap
typedef string FruName

// The configuration for sensor mapping.
struct SensorConfig {
  2: map<FruName, sensorMap> sensorMapList;
}
