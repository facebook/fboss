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

// Service Config needs to access sensor data
#include "SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

#define FSVC_DEFAULT_SENSOR_FETCH_FREQUENCY 30
#define FSVC_DEFAULT_CONTROL_FREQUENCY 30
#define FSVC_DEFAULT_PWM_LOWER_THRES 15
#define FSVC_DEFAULT_PWM_UPPER_THRES 85

namespace facebook::fboss::platform {

std::string getDarwinFSConfig();
std::string getMokujinFSConfig();

class FanStatus {
 public:
  int rpm;
  float currentPwm{0};
  bool fanFailed{false};
  bool fanAccessLost{false};
  bool firstTimeLedAccess{true};
  uint64_t timeStamp;
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

class PwmCalcCache {
 public:
  float previousSensorRead{0};
  float previousTargetPwm{0};
  float previousRead1{0};
  float previousRead2{0};
  float integral{0};
  float last_error{0};
};

class ServiceConfig {
 public:
  //
  // Attribs
  //
  std::vector<fan_config_structs::Zone> zones;
  std::vector<fan_config_structs::Sensor> sensors;
  std::map<std::string /* sensorName */, SensorReadCache> sensorReadCaches;
  std::map<std::string /* sensorName */, PwmCalcCache> pwmCalcCaches;
  std::vector<fan_config_structs::Optic> optics;
  std::vector<fan_config_structs::Fan> fans;
  std::map<std::string /* fanName */, FanStatus> fanStatuses;

  std::optional<fan_config_structs::Watchdog> watchdog_{std::nullopt};
  // Number of broken fan required for pwm boost
  int pwmBoostOnDeadFan;
  // Number of broken fan required for pwm boost
  int pwmBoostOnDeadSensor;
  // When no QSFP data present, after how many seconds
  // boost mode starts. ( 0 = disable this feature)
  int pwmBoostNoQsfpAfterInSec;
  // BSP Type
  fan_config_structs::BspType bspType;

  //
  // Methods
  //

  ServiceConfig();
  int parse();
  fan_config_structs::FsvcConfigDictIndex convertKeywordToIndex(
      std::string keyword) const;
  float getPwmBoostValue() const;
  std::string getShutDownCommand() const;
  int getSensorFetchFrequency() const;
  int getControlFrequency() const;
  int getPwmUpperThreshold() const;
  int getPwmLowerThreshold() const;
  float getPwmTransitionValue() const;
  int parseConfigString(std::string contents);
  std::optional<fan_config_structs::TempToPwmMap> getConfigOpticTable(
      std::string name,
      fan_config_structs::OpticTableType dataType);

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
  // % Pwm at fan_service start
  float pwmTransitionValue_;

  // PWM Limit as percent
  int pwmUpperThreshold_;
  int pwmLowerThreshold_;

  // Frequency
  int sensorFetchFrequency_;
  int controlFrequency_;

  // Eanble Watchdog
  bool watchdogEnable_;
  fan_config_structs::AccessMethod watchdogAccess_;
  std::string watchdogValue_;

  // TODO - just use thrift enum names
  std::unordered_map<std::string, fan_config_structs::FsvcConfigDictIndex>
      configDict_;

  //
  // Methods
  //
  std::string getConfigContents();
  fan_config_structs::AccessMethod parseAccessMethod(folly::dynamic value);
  std::vector<std::pair<float, float>> parseTable(folly::dynamic value);
  fan_config_structs::Alarm parseAlarm(folly::dynamic value);
  fan_config_structs::RangeCheck parseRangeCheck(folly::dynamic value);
  void parseZonesChapter(folly::dynamic value);
  void parseFansChapter(folly::dynamic value);
  void parseSensorsChapter(folly::dynamic value);
  void parseOpticsChapter(folly::dynamic values);
  void parseWatchdogChapter(folly::dynamic values);
  void prepareDict();
  void parseBspType(std::string bspString);
};
} // namespace facebook::fboss::platform
