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
  auto static constexpr kReadTotal = "sensor_read.total";
  auto static constexpr kTotalReadFailure = "sensor_read.total.failures";
  auto static constexpr kHasReadFailure = "sensor_read.has.failures";
  auto static constexpr kCriticalThresholdViolation =
      "sensor_read.sensor_{}.type_{}.critical_threshold_violation";
  auto static constexpr kAsicTemp = "asic_temp";
  auto static constexpr kTotalPower = "TOTAL_POWER";
  auto static constexpr kMaxInputVoltage = "MAX_INPUT_VOLTAGE";
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

  void publishPerSensorStats(
      const std::string& sensorName,
      std::optional<float> value);

  void publishDerivedStats(
      const std::string& entity,
      std::optional<float> value);

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
  std::shared_ptr<Utils> utils_{};
  std::shared_ptr<PlatformUtils> platformUtils_{};
  PmUnitInfoFetcher pmUnitInfoFetcher_{};
};

} // namespace facebook::fboss::platform::sensor_service
