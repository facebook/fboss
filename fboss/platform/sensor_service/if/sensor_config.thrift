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

// `PmSensor`: Describes a sensor in PmUnit.
//
// `name`: Name of the sensor. This isn't neccessarily same as PmUnitScopedName in PM config.
//
// `sysfsPath`: Sensor's sysfs path in /run/devmap/. E.g /run/devmap/sensors/MCB_SENSOR1/...
//
// `thresholds`: Manufacture provided threshold values
//
// `compute`: Compute method, same format and calculation approach as lm_sensor, e.g. @*0.1
//
// `type`: See SensorType definition above.
struct PmSensor {
  1: string name;
  2: string sysfsPath;
  3: optional Thresholds thresholds;
  4: optional string compute;
  5: SensorType type;
}

// `VersionedPmSensor`: Describes a set of sensors which would exist in Platforms with
// minimum productProductionState, productVersion and productSubVersion.
//
// `sensors`: A set of sensors belong to this version. They're mutually exclusive from other versions.
// If there're any carry-over sensors in the other versions, they must be redefined in that version.
//
// `productProductionState`: Minimum productProductionState (EEPROM V5 Type 8).
//
// `productVersion`: Minimum productVersion (EEPROM V5 Type 9).
//
// `productSubVersion`: Minimum productSubVersion (EEPROM V5 Type 10).
struct VersionedPmSensor {
  1: list<PmSensor> sensors;
  2: i16 productProductionState;
  3: i16 productVersion;
  4: i16 productSubVersion;
}

// `PmUnitSensors`: Describes every sensor in PmUnit.
//
// `slotPath`: Refers to the location of slot in the platform.
//
// `pmUnitName`: Name of the PmUnit.
//
// `sensors`: List of common pmSensor across respins. See above PmSensor definition.
//
// `versionedSensors`: List of versionedPmSensors for specific respin.
// See above VersionedPmSensor definition.
struct PmUnitSensors {
  1: string slotPath;
  2: string pmUnitName;
  3: list<PmSensor> sensors;
  4: list<VersionedPmSensor> versionedSensors;
}

// The configuration for sensor mapping.
struct SensorConfig {
  1: list<PmUnitSensors> pmUnitSensorsList;
}
