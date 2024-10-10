/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <filesystem>

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/gen-cpp2/sensor_service_stats_types.h"

DEFINE_int32(
    fsdb_statsStream_interval_seconds,
    5,
    "Interval at which stats subscriptions are served");

namespace facebook::fboss::platform::sensor_service {

SensorServiceImpl::SensorServiceImpl() {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  std::string sensorConfJson = ConfigLib().getSensorServiceConfig(platformName);
  XLOG(DBG2) << "Read sensor config: " << sensorConfJson;
  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorConfJson, sensorConfig_);
  fsdbSyncer_ = std::make_unique<FsdbSyncer>();
}

SensorServiceImpl::~SensorServiceImpl() {
  if (fsdbSyncer_) {
    fsdbSyncer_->stop();
  }
  fsdbSyncer_.reset();
}

std::vector<SensorData> SensorServiceImpl::getSensorsData(
    const std::vector<std::string>& sensorNames) {
  std::vector<SensorData> sensorDataVec;
  auto allSensorData = getAllSensorData();
  for (const auto& sensorName : sensorNames) {
    auto it = allSensorData.find(sensorName);
    if (it != allSensorData.end()) {
      sensorDataVec.push_back(it->second);
    }
  }
  return sensorDataVec;
}

std::map<std::string, SensorData> SensorServiceImpl::getAllSensorData() {
  return polledData_.copy();
}

void SensorServiceImpl::fetchSensorData() {
  std::map<std::string, SensorData> polledData;
  uint readFailures{0};
  XLOG(INFO) << "Reading SensorData using PM based sensor structs...";
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    auto pmSensors = resolveSensors(pmUnitSensors);
    XLOG(INFO) << fmt::format(
        "Processing {} unit {} sensors",
        *pmUnitSensors.pmUnitName(),
        pmSensors.size());
    for (const auto& sensor : pmSensors) {
      const auto& sensorName = *sensor.name();
      auto sensorData = fetchSensorDataImpl(
          sensorName,
          *sensor.sysfsPath(),
          *sensor.type(),
          sensor.thresholds().to_optional(),
          sensor.compute().to_optional());
      polledData[sensorName] = sensorData;
      // We log 0 if there is a read failure.  If we dont log 0 on failure,
      // fb303 will pick up the last reported (on read success) value and
      // keep reporting that as the value. For 0 values, it is accurate to
      // read the value along with the kReadFailure counter. Alternative is
      // to delete this counter if there is a failure.
      fb303::fbData->setCounter(
          fmt::format(kReadValue, sensorName), sensorData.value().value_or(0));
      if (!sensorData.value()) {
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 1);
        readFailures++;
      } else {
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 0);
      }
    }
  }
  fb303::fbData->setCounter(kReadTotal, polledData.size());
  fb303::fbData->setCounter(kTotalReadFailure, readFailures);
  fb303::fbData->setCounter(kHasReadFailure, readFailures > 0 ? 1 : 0);
  XLOG(INFO) << fmt::format(
      "In Total, Processed {} Sensors. {} Failures.",
      polledData.size(),
      readFailures);
  polledData_.swap(polledData);

  if (FLAGS_publish_stats_to_fsdb) {
    auto now = std::chrono::steady_clock::now();
    if (!publishedStatsToFsdbAt_ ||
        std::chrono::duration_cast<std::chrono::seconds>(
            now - *publishedStatsToFsdbAt_)
                .count() >= FLAGS_fsdb_statsStream_interval_seconds) {
      stats::SensorServiceStats sensorServiceStats;
      sensorServiceStats.sensorData() = getAllSensorData();
      fsdbSyncer_->statsUpdated(std::move(sensorServiceStats));
      publishedStatsToFsdbAt_ = now;
    }
  }
}

std::vector<PmSensor> SensorServiceImpl::resolveSensors(
    const PmUnitSensors& pmUnitSensors) {
  auto pmSensors = *pmUnitSensors.sensors();
  if (auto versionedPmSensors = Utils().resolveVersionedSensors(
          pmUnitInfoFetcher_,
          *pmUnitSensors.slotPath(),
          *pmUnitSensors.versionedSensors())) {
    XLOG(INFO) << fmt::format(
        "Resolved to versionedPmSensors config with version {}.{}.{} for pmUnit {} at {}",
        *versionedPmSensors->productProductionState(),
        *versionedPmSensors->productVersion(),
        *versionedPmSensors->productSubVersion(),
        *pmUnitSensors.pmUnitName(),
        *pmUnitSensors.slotPath());
    pmSensors.insert(
        pmSensors.end(),
        versionedPmSensors->sensors()->begin(),
        versionedPmSensors->sensors()->end());
  }
  return pmSensors;
}

SensorData SensorServiceImpl::fetchSensorDataImpl(
    const std::string& sensorName,
    const std::string& sysfsPath,
    SensorType sensorType,
    const std::optional<Thresholds>& thresholds,
    const std::optional<std::string>& compute) {
  SensorData sensorData{};
  sensorData.name() = sensorName;
  std::string sensorValue;
  bool sysfsFileExists =
      std::filesystem::exists(std::filesystem::path(sysfsPath));
  if (sysfsFileExists && folly::readFile(sysfsPath.c_str(), sensorValue)) {
    sensorData.value() = folly::to<float>(sensorValue);
    sensorData.timeStamp() = Utils::nowInSecs();
    if (compute) {
      sensorData.value() =
          Utils::computeExpression(*compute, *sensorData.value());
    }
    XLOG(DBG1) << fmt::format(
        "{} ({}) : {}", sensorName, sysfsPath, *sensorData.value());
  } else {
    XLOG(ERR) << fmt::format(
        "Could not read data for {} from path:{}, error:{}",
        sensorName,
        sysfsPath,
        sysfsFileExists ? folly::errnoStr(errno) : "File does not exist");
  }
  sensorData.thresholds() = thresholds ? *thresholds : Thresholds();
  sensorData.sensorType() = sensorType;
  return sensorData;
}

} // namespace facebook::fboss::platform::sensor_service
