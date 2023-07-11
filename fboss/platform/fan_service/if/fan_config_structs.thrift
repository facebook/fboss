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
  kFsvcCfgFans = 20,
  kFsvcCfgFanPwm = 21,
  kFsvcCfgFanRpm = 22,
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
  kFsvcCfgFanLed = 58,
  kFsvcCfgFanGoodLedVal = 59,
  kFsvcCfgFanFailLedVal = 60,
  kFsvcCfgFanPresence = 61,
  kFsvcCfgFanPresentVal = 62,
  kFsvcCfgFanMissingVal = 63,
  kFsvcCfgOptics = 64,
  kFsvcCfgNoQsfpBoostInSec = 72,
  kFsvcCfgPwmRangeMin = 73,
  kFsvcCfgPwmRangeMax = 74,
  kFsvcCfgPwmTransition = 75,
  kFsvcCfgValue = 77,
  kFsvcCfgScale = 78,
  kFsvcCfgBspSandia = 79,
}

enum ZoneType {
  kZoneMax = 0,
  kZoneMin = 1,
  kZoneAvg = 2,
  kZoneInval = 3,
}

enum OpticAggregationType {
  kOpticMax = 0,
  kOpticInval = 1,
}

enum OpticTableType {
  kOpticTable100Generic = 0,
  kOpticTable200Generic = 1,
  kOpticTable400Generic = 2,
  kOpticTable800Generic = 3,
  kOpticTableInval = 4,
}

enum SourceType {
  kSrcSysfs = 0,
  kSrcUtil = 1,
  kSrcThrift = 2,
  kSrcRest = 3,
  kSrcInvalid = 4,
  kSrcQsfpService = 5,
}

struct AccessMethod {
  1: SourceType accessType = kSrcInvalid;
  2: string path;
}

struct Zone {
  1: ZoneType zoneType;
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
  4: OpticAggregationType aggregationType;
  5: map<OpticTableType, TempToPwmMap> tempToPwmMaps;
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
