/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <vector>

#include <folly/Synchronized.h>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/helpers/StructuredLogger.h"
#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/PmUnitInfoFetcher.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

DECLARE_int32(fsdb_statsStream_interval_seconds);

namespace facebook::fboss::platform::sensor_service {
using namespace facebook::fboss::platform::sensor_config;

class SensorServiceImpl {
 public:
  auto static constexpr kReadFailure = "sensor_read.{}.failure";
  auto static constexpr kReadValue = "sensor_read.{}.value";
  auto static constexpr kCriticalThresholdViolation =
      "sensor_read.{}.critical_threshold_violation";
  auto static constexpr kAlarmThresholdViolation =
      "sensor_read.{}.alarm_threshold_violation";
  auto static constexpr kReadTotal = "sensor_read.total";
  auto static constexpr kTotalReadFailure = "sensor_read.total.failures";
  auto static constexpr kHasReadFailure = "sensor_read.has.failures";
  auto static constexpr kAggHasCriticalThresholdViolation =
      "sensor_read.agg.{}.has.critical_threshold_violation";
  auto static constexpr kAggHasAlarmThresholdViolation =
      "sensor_read.agg.{}.has.alarm_threshold_violation";
  auto static constexpr kTotalPower = "TOTAL_POWER";
  auto static constexpr kMaxInputVoltage = "MAX_INPUT_VOLTAGE";
  auto static constexpr kInputPowerType = "INPUT_POWER_TYPE";
  static constexpr int kInputPowerTypeUnknown = 0;
  static constexpr int kInputPowerTypeDC = 1;
  static constexpr int kInputPowerTypeAC = 2;
  static constexpr float kACVoltageThreshold = 80.0f;
  static constexpr float kMinVoltageThreshold = 9.0f;
  auto static constexpr kDerivedFailure = "derived.{}.failure";
  auto static constexpr kDerivedValue = "derived.{}.value";
  auto static constexpr kPmUnitVersion = "pmunit.{}.version.{}.{}.{}";

  explicit SensorServiceImpl(
      const SensorConfig& sensorConfig,
      const std::shared_ptr<Utils>& utils = std::make_shared<Utils>(),
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>());
  ~SensorServiceImpl();

  std::vector<SensorData> getSensorsData(
      const std::vector<std::string>& sensorNames);
  std::map<std::string, SensorData> getAllSensorData();
  void fetchSensorData();
  std::vector<PmSensor> resolveSensors(const PmUnitSensors& pmUnitSensors);

  FsdbSyncer* fsdbSyncer() {
    return fsdbSyncer_.get();
  }

  void processPower(
      const std::map<std::string, SensorData>& polledData,
      const PowerConfig& powerConfig);

  void processTemperature(
      const std::map<std::string, SensorData>& polledData,
      const std::vector<TemperatureConfig>& tempConfigs);

  void processInputVoltage(
      const std::map<std::string, SensorData>& polledData,
      const std::vector<std::string>& inputVoltageSensors);

  SensorData processAsicCmd(const AsicCommand& asicCommand);

 private:
  SensorData fetchSensorDataImpl(
      const std::string& name,
      const std::string& sysfsPath,
      SensorType sensorType,
      const std::optional<Thresholds>& thresholds,
      const std::optional<std::string>& compute);

  void publishPerSensorStats(const SensorData& sensorData);

  void publishDerivedStats(
      const std::string& entity,
      std::optional<float> value);

  void publishAggStats(const std::map<std::string, SensorData>& polledData);

  void publishVersionedSensorStats(
      const std::string& pmUnitName,
      int16_t productProductionState,
      int16_t productVersion,
      int16_t productSubVersion);

  folly::Synchronized<std::map<std::string, SensorData>> polledData_{};
  std::unique_ptr<FsdbSyncer> fsdbSyncer_;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>>
      publishedStatsToFsdbAt_;
  SensorConfig sensorConfig_{};
  helpers::StructuredLogger structuredLogger_;
  std::shared_ptr<Utils> utils_{};
  std::shared_ptr<PlatformUtils> platformUtils_{};
  PmUnitInfoFetcher pmUnitInfoFetcher_{};
  int inputPowerType_{kInputPowerTypeUnknown};
};

} // namespace facebook::fboss::platform::sensor_service
