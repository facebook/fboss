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

#include <memory>
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
  auto static constexpr kHasCriticalThresholdViolation =
      "sensor_read.has.critical_threshold_violation";
  auto static constexpr kHasAlarmThresholdViolation =
      "sensor_read.has.alarm_threshold_violation";
  auto static constexpr kTotalPower = "TOTAL_POWER";
  auto static constexpr kMaxInputVoltage = "MAX_INPUT_VOLTAGE";
  // Flat fb303 counters (NOT under derived.{}.value — these are observed
  // and config-derived, not computed). The boolean fires when present
  // count drops below the configured min for the detected power type.
  auto static constexpr kTotalNumPresentPsu = "psu.total_num_present";
  auto static constexpr kUnexpectedNumPresentPsu =
      "psu.unexpected_num_present_psu";
  auto static constexpr kInputPowerType = "INPUT_POWER_TYPE";
  static constexpr int kInputPowerTypeUnknown = 0;
  static constexpr int kInputPowerTypeDC = 1;
  static constexpr int kInputPowerTypeAC = 2;
  auto static constexpr kDerivedFailure = "derived.{}.failure";
  auto static constexpr kDerivedValue = "derived.{}.value";
  auto static constexpr kPmUnitVersion = "pmunit.{}.version.{}.{}.{}";

  explicit SensorServiceImpl(
      const SensorConfig& sensorConfig,
      const std::shared_ptr<Utils>& utils = std::make_shared<Utils>(),
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>());
  ~SensorServiceImpl();

  void setPmUnitInfoFetcherForTest(std::unique_ptr<PmUnitInfoFetcher> fetcher) {
    pmUnitInfoFetcher_ = std::move(fetcher);
  }

  std::vector<SensorData> getSensorsData(
      const std::vector<std::string>& sensorNames);
  std::map<std::string, SensorData> getAllSensorData();
  void fetchSensorData();
  std::vector<PmSensor> resolveSensors(const PmUnitSensors& pmUnitSensors);

  FsdbSyncer* fsdbSyncer() {
    return fsdbSyncer_.get();
  }

  // pmUnitInfoMap (default empty): per-slot PmUnitInfo from
  // prefetchPmUnitInfos. processPower consults it to skip per-slot _POWER
  // publication when platform_manager reports a PSU/PEM slot absent. The
  // amended D98824972 validator guarantees PSU/PEM PerSlotPowerConfigs
  // always have a non-empty slotPath that resolves to a known
  // PmUnitSensors.slotPath, so the lookup is well-defined.
  void processPower(
      const std::map<std::string, SensorData>& polledData,
      const PowerConfig& powerConfig,
      const std::map<std::string, std::optional<platform_manager::PmUnitInfo>>&
          pmUnitInfoMap = {});

  void processTemperature(
      const std::map<std::string, SensorData>& polledData,
      const std::vector<TemperatureConfig>& tempConfigs);

  void processInputVoltage(
      std::map<std::string, SensorData>& polledData,
      const PowerConfig& powerConfig);

  // Publishes psu.total_num_present + psu.unexpected_num_present_psu by
  // counting physical PSU/PEM slots from pmUnitSensorsList (one entry per
  // physical slot) — NOT perSlotPowerConfigs (which can have multiple
  // logical entries per physical slot, e.g. Darwin's PEM1/PEM2 →
  // /PEM_SLOT@0). Must be called after processInputVoltage so
  // inputPowerType_ is resolved.
  void publishPsuPemCounters(
      const std::map<std::string, std::optional<platform_manager::PmUnitInfo>>&
          pmUnitInfoMap,
      const PowerConfig& powerConfig);

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

  // Pre-fetches PmUnitInfo for every PmUnitSensors entry that declares
  // versionedSensors OR is a field-replaceable PSU/PEM slot, so the
  // version resolver, fetchSensorData's skip-absent check, processPower,
  // and publishPsuPemCounters all share one RPC per slot per cycle.
  std::map<std::string, std::optional<platform_manager::PmUnitInfo>>
  prefetchPmUnitInfos();

  folly::Synchronized<std::map<std::string, SensorData>> polledData_{};
  std::unique_ptr<FsdbSyncer> fsdbSyncer_;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>>
      publishedStatsToFsdbAt_;
  SensorConfig sensorConfig_{};
  helpers::StructuredLogger structuredLogger_;
  std::shared_ptr<Utils> utils_{};
  std::shared_ptr<PlatformUtils> platformUtils_{};
  std::unique_ptr<PmUnitInfoFetcher> pmUnitInfoFetcher_ =
      std::make_unique<PmUnitInfoFetcher>();
  int inputPowerType_{kInputPowerTypeUnknown};
};

} // namespace facebook::fboss::platform::sensor_service
