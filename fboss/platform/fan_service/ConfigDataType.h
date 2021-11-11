// Copyright 2021- Facebook. All rights reserved.

// This is a headerfile containing the definition of datatypes
// used by ServiceConfig class

#pragma once
namespace facebook::fboss::platform {
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
} // namespace facebook::fboss::platform
