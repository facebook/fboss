namespace cpp2 facebook.fboss.platform.fan_config_structs
namespace py neteng.fboss.platform.fan_config_structs
namespace py3 neteng.fboss.platform.fan_config_structs
namespace py.asyncio neteng.fboss.platform.asyncio.fan_config_structs
namespace go neteng.fboss.platform.fan_config_structs

enum FsvcConfigDictIndex {
  kFsvcCfgInvalid = 0,
  kFsvcCfgBsp = 1,
  kFsvcCfgBspGeneric = 2,
  kFsvcCfgBspDarwin = 3,
  kFsvcCfgBspMokujin = 4,
  kFsvcCfgBspLassen = 5,
  kFsvcCfgBspMinipack3 = 6,
  kFsvcCfgPwmBoost = 7,
  kFsvcCfgBoostOnDeadFan = 8,
  kFsvcCfgBoostOnDeadSensor = 9,
  kFsvcCfgPwmUpper = 10,
  kFsvcCfgPwmLower = 11,
  kFscvCfgWatchdogEnable = 12,
  kFsvcCfgShutdownCmd = 13,
  kFsvcCfgChapterZones = 14,
  kFsvcCfgZonesName = 15,
  kFsvcCfgZonesType = 16,
  kFsvcCfgZonesTypeMax = 17,
  kFsvcCfgZonesTypeMin = 18,
  kFsvcCfgZonesTypeAvg = 19,
  kFsvcCfgFans = 20,
  kFsvcCfgFanPwm = 21,
  kFsvcCfgFanRpm = 22,
  kFsvcCfgZonesFanSlope = 23,
  kFsvcCfgSource = 24,
  kFsvcCfgSourceSysfs = 25,
  kFsvcCfgSourceUtil = 26,
  kFsvcCfgSourceThrift = 27,
  kFsvcCfgSourceRest = 28,
  kFsvcCfgAccessPath = 29,
  kFsvcCfgSensors = 30,
  kFsvcCfgAccess = 31,
  kFsvcCfgSensorAdjustment = 32,
  kFsvcCfgSensorAlarm = 33,
  kFsvcCfgAlarmMajor = 34,
  kFsvcCfgAlarmMinor = 35,
  kFsvcCfgAlarmMinorSoakInSec = 36,
  kFsvcCfgRangeCheck = 37,
  kFsvcCfgRangeLow = 38,
  kFsvcCfgRangeHigh = 39,
  kFsvcCfgInvalidRangeTolerance = 40,
  kFsvcCfgInvalidRangeAction = 41,
  kFsvcCfgInvalidRangeActionShutdown = 42,
  kFsvcCfgInvalidRangeActionNone = 43,
  kFsvcCfgSensorType = 44,
  kFsvcCfgSensorType4Cuv = 45,
  kFsvcCfgSensorTypeIncrementPid = 46,
  kFsvcCfgSensorTypePid = 47,
  kFsvcCfgSensor4CuvUp = 48,
  kFsvcCfgSensor4CuvDown = 49,
  kFsvcCfgSensor4CuvFailUp = 50,
  kFsvcCfgSensor4CuvFailDown = 51,
  kFsvcCfgSensorIncrpidSetpoint = 52,
  kFsvcCfgSensorIncrpidPosHyst = 53,
  kFsvcCfgSensorIncrpidNegHyst = 54,
  kFsvcCfgSensorIncrpidKp = 55,
  kFsvcCfgSensorIncrpidKi = 56,
  kFsvcCfgSensorIncrpidKd = 57,
}

enum ZoneType {
  kZoneMax = 0,
  kZoneMin = 1,
  kZoneAvg = 2,
  kZoneInval = 3,
}

enum SourceType {
  kSrcSysfs = 0,
  kSrcUtil = 1,
  kSrcThrift = 2,
  kSrcRest = 3,
  kSrcInvalid = 4,
}

struct AccessMethod {
  1: SourceType accessType = kSrcInvalid;
  2: string path;
}

enum BspType {
  kBspGeneric = 0,
  kBspDarwin = 1,
  kBspLassen = 2,
  kBspMinipack3 = 3,
  kBspMokujin = 4,
}

enum SensorPwmCalcType {
  kSensorPwmCalcFourLinearTable = 0,
  kSensorPwmCalcIncrementPid = 1,
  kSensorPwmCalcPid = 2,
  kSensorPwmCalcDisable = 3,
}
