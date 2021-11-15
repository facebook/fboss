// Copyright 2021- Facebook. All rights reserved.

// ServiceConfig class is a part of Fan Service
// Role : Parse the config file, and store the parsed setting.
//        This class also stores some of the status of sensors needed for
//        applying the actions defined by the config file.

#pragma once

// Standard C++ functions
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

// Folly and logging
#include <folly/json.h>
#include <folly/logging/xlog.h>

// Reuse facebook::fboss::FbossError for consistency
#include "fboss/agent/FbossError.h"

// Fundamental data types and enums are defined here
#include "fboss/platform/fan_service/ConfigDataType.h"
// Service Config needs to access sensor data
#include "SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

#define FSVC_DEFAULT_SENSOR_FETCH_FREQUENCY 30
#define FSVC_DEFAULT_CONTROL_FREQUENCY 30
#define FSVC_DEFAULT_PWM_LOWER_THRES 15
#define FSVC_DEFAULT_PWM_UPPER_THRES 85

namespace facebook::fboss::platform {

class AccessMethod {
 public:
  SourceType accessType;
  std::string path;
  unsigned int min;
  unsigned int max;
  AccessMethod() {
    min = 0;
    max = 255;
    accessType = kSrcInvalid;
  }
};

class Zone {
 public:
  ZoneType type;
  std::string zoneName;
  std::vector<std::string> sensorNames;
  std::vector<std::string> fanNames;
  float slope;
  Zone() {
    // 0 means immediate
    slope = 0;
  }
};

class FanStatus {
 public:
  int rpm;
  float currentPwm;
  bool fanFailed;
  bool fanAccessLost;
  uint64_t timeStamp;
  FanStatus() {
    fanFailed = false;
    fanAccessLost = false;
    currentPwm = 0;
  }
};

class Fan {
 public:
  std::string fanName;
  std::string label;
  // Fan RPM is read as a sensor, but referred here
  // through the sensor name
  std::string rpmSensorName;
  AccessMethod pwm;
  AccessMethod rpmAccess;
  unsigned int pwmMin;
  unsigned int pwmMax;
  FanStatus fanStatus;
  int fanFailThresholdInSec;

  Fan() {
    pwmMin = 0;
    pwmMax = 255;
    fanFailThresholdInSec = 300;
  }
};

class Alarm {
 public:
  float high_minor;
  int high_minor_soak;
  float high_major;
  bool enabled;
  Alarm() {
    enabled = false;
    high_minor = 0;
    high_major = 0;
    high_minor_soak = 300;
  }
};

typedef enum {
  kRangeCheckActionNone,
  kRangeCheckActionShutdown,
  kRangeCheckActionInvalid
} RangeCheckActionType;

class RangeCheck {
 public:
  float rangeLow;
  float rangeHigh;
  int tolerance;
  bool enabled;
  RangeCheckActionType action;
  int invalidCount;
  RangeCheck() {
    invalidCount = 0;
    enabled = false;
    action = kRangeCheckActionInvalid;
    rangeLow = 0.0;
    rangeHigh = 0.0;
    tolerance = 0;
  }
};

class SensorReadCache {
 public:
  float adjustedReadCache;
  float targetPwmCache;
  uint64_t lastUpdatedTime;
  bool enabled;
  bool soakStarted;
  uint64_t sensorAccessLostAt;
  bool sensorFailed;
  bool minorAlarmTriggered;
  bool majorAlarmTriggered;
  uint64_t soakStartedAt;
  SensorReadCache() {
    enabled = false;
    soakStarted = false;
    targetPwmCache = 0;
    minorAlarmTriggered = false;
    majorAlarmTriggered = false;
  }
};

class FourCurves {
 public:
  std::vector<std::pair<float, float>> normalUp;
  std::vector<std::pair<float, float>> normalDown;
  std::vector<std::pair<float, float>> failUp;
  std::vector<std::pair<float, float>> failDown;
  float previousSensorRead;
  FourCurves() {
    previousSensorRead = 0;
  }
  ~FourCurves() {} // Make unique_ptr happy
};

class IncrementPid {
 public:
  float setPoint;
  float posHysteresis;
  float negHysteresis;
  float kp;
  float kd;
  float ki;
  float previousTargetPwm = 0;
  float previousRead1 = 0;
  float previousRead2 = 0;
  float minVal;
  float maxVal;
  float i;
  float last_error;
  IncrementPid() {
    previousTargetPwm = 0;
    previousRead1 = 0;
    previousRead2 = 0;
    posHysteresis = 0;
    negHysteresis = 0;
    i = 0;
    last_error = 0;
  }
  void updateMinMaxVal() {
    minVal = setPoint - negHysteresis;
    maxVal = setPoint + posHysteresis;
  }
  ~IncrementPid() {} // Make unique_ptr happy
};

class Sensor {
 public:
  std::string sensorName;
  AccessMethod access;
  std::vector<std::pair<float, float>> offsetTable;
  Alarm alarm;
  RangeCheck rangeCheck;
  SensorPwmCalcType calculationType;
  IncrementPid incrementPid;
  FourCurves fourCurves;
  int sensorFailThresholdInSec;
  SensorReadCache processedData;
  Sensor() {
    sensorFailThresholdInSec = 300;
    calculationType = kSensorPwmCalcDisable;
  }
};

class ServiceConfig {
 public:
  //
  // Attribs
  //
  std::vector<Zone> zones;
  std::vector<Sensor> sensors;
  std::vector<Fan> fans;
  // Number of broken fan required for pwm boost
  int pwmBoostOnDeadFan;
  // Number of broken fan required for pwm boost
  int pwmBoostOnDeadSensor;
  // BSP Type
  BspType bspType;

  //
  // Methods
  //

  ServiceConfig();
  int parse(std::string filename);
  fan_config_structs::FsvcConfigDictIndex convertKeywordToIndex(
      std::string keyword) const;
  float getPwmBoostValue() const;
  std::string getShutDownCommand() const;
  int getSensorFetchFrequency() const;
  int getControlFrequency() const;
  int getPwmUpperThreshold() const;
  int getPwmLowerThreshold() const;

 private:
  //
  // Attribs
  //
  folly::dynamic jsonConfig_;
  // SensorData Storage
  SensorData sensorData_;
  // Shutdown Command
  std::string shutDownCommand_;
  // % Fan Boost upon fan failure
  float pwmBoostValue_;

  // PWM Limit as percent
  int pwmUpperThreshold_;
  int pwmLowerThreshold_;

  // Frequency
  int sensorFetchFrequency_;
  int controlFrequency_;

  // Eanble Watchdog
  bool watchdog_;
  // TODO - just use thrift enum names
  std::unordered_map<std::string, fan_config_structs::FsvcConfigDictIndex>
      configDict_;

  //
  // Methods
  //
  AccessMethod parseAccessMethod(folly::dynamic value);
  std::vector<std::pair<float, float>> parseTable(folly::dynamic value);
  Alarm parseAlarm(folly::dynamic value);
  RangeCheck parseRangeCheck(folly::dynamic value);
  void parseZonesChapter(folly::dynamic value);
  void parseFansChapter(folly::dynamic value);
  void parseSensorsChapter(folly::dynamic value);
  void prepareDict();
  void parseBspType(std::string bspString);
};
} // namespace facebook::fboss::platform
