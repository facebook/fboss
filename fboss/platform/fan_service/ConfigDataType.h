// Copyright 2021- Facebook. All rights reserved.

// This is a headerfile containing the definition of datatypes
// used by ServiceConfig class

#pragma once
typedef enum {
  kFsvcCfgInvalid,
  kFsvcCfgBsp,
  kFsvcCfgBspGeneric,
  kFsvcCfgBspDarwin,
  kFsvcCfgBspMokujin,
  kFsvcCfgBspLassen,
  kFsvcCfgBspMinipack3,
  kFsvcCfgPwmBoost,
  kFsvcCfgBoostOnDeadFan,
  kFsvcCfgBoostOnDeadSensor,
  kFsvcCfgPwmUpper,
  kFsvcCfgPwmLower,
  kFscvCfgWatchdogEnable,
  kFsvcCfgShutdownCmd,
  kFsvcCfgChapterZones,
  kFsvcCfgZonesName,
  kFsvcCfgZonesType,
  kFsvcCfgZonesTypeMax,
  kFsvcCfgZonesTypeMin,
  kFsvcCfgZonesTypeAvg,
  kFsvcCfgFans,
  kFsvcCfgFanPwm,
  kFsvcCfgFanRpm,
  kFsvcCfgZonesFanSlope,
  kFsvcCfgSource,
  kFsvcCfgSourceSysfs,
  kFsvcCfgSourceUtil,
  kFsvcCfgSourceThrift,
  kFsvcCfgSourceRest,
  kFsvcCfgAccessPath,
  kFsvcCfgSensors,
  kFsvcCfgAccess,
  kFsvcCfgSensorAdjustment,
  kFsvcCfgSensorAlarm,
  kFsvcCfgAlarmMajor,
  kFsvcCfgAlarmMinor,
  kFsvcCfgAlarmMinorSoakInSec,
  kFsvcCfgRangeCheck,
  kFsvcCfgRangeLow,
  kFsvcCfgRangeHigh,
  kFsvcCfgInvalidRangeTolerance,
  kFsvcCfgInvalidRangeAction,
  kFsvcCfgInvalidRangeActionShutdown,
  kFsvcCfgInvalidRangeActionNone,
  kFsvcCfgSensorType,
  kFsvcCfgSensorType4Cuv,
  kFsvcCfgSensorTypeIncrementPid,
  kFsvcCfgSensorTypePid,
  kFsvcCfgSensor4CuvUp,
  kFsvcCfgSensor4CuvDown,
  kFsvcCfgSensor4CuvFailUp,
  kFsvcCfgSensor4CuvFailDown,
  kFsvcCfgSensorIncrpidSetpoint,
  kFsvcCfgSensorIncrpidPosHyst,
  kFsvcCfgSensorIncrpidNegHyst,
  kFsvcCfgSensorIncrpidKp,
  kFsvcCfgSensorIncrpidKi,
  kFsvcCfgSensorIncrpidKd
} FsvcConfigDictIndex;

typedef enum { kZoneMax, kZoneMin, kZoneAvg, kZoneInval } ZoneType;

typedef enum {
  kSrcSysfs,
  kSrcUtil,
  kSrcThrift,
  kSrcRest,
  kSrcInvalid
} SourceType;

typedef enum {
  kBspGeneric,
  kBspDarwin,
  kBspLassen,
  kBspMinipack3,
  kBspMokujin
} BspType;

typedef enum {
  kSensorPwmCalcFourLinearTable,
  kSensorPwmCalcIncrementPid,
  kSensorPwmCalcPid,
  kSensorPwmCalcDisable
} SensorPwmCalcType;
