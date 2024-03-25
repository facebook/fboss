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
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/gen-cpp2/sensor_service_stats_types.h"

DEFINE_int32(
    fsdb_statsStream_interval_seconds,
    5,
    "Interval at which stats subscriptions are served");

namespace {
auto constexpr kReadFailure = "sensor_read.{}.failure";
auto constexpr kReadValue = "sensor_read.{}.value";
auto constexpr kReadTotal = "sensor_read.total";
auto constexpr kTotalReadFailure = "sensor_read.total.failures";
auto constexpr kHasReadFailure = "sensor_read.has.failures";
} // namespace

namespace facebook::fboss::platform::sensor_service {

SensorServiceImpl::SensorServiceImpl() {
  std::string sensorConfJson = ConfigLib().getSensorServiceConfig();
  XLOG(DBG2) << "Read sensor config: " << sensorConfJson;
  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorConfJson, sensorConfig_);
  for (const auto& [fruName, sensorMap] : *sensorConfig_.sensorMapList()) {
    for (const auto& [sensorName, sensor] : sensorMap) {
      XLOG(INFO) << fmt::format(
          "{}: Path={}, Compute={}, FRU={}",
          sensorName,
          *sensor.path(),
          sensor.compute().value_or(""),
          fruName);
    }
  }
  fsdbSyncer_ = std::make_unique<FsdbSyncer>();
  XLOG(INFO) << "========================================================";
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
  auto nowTs = Utils::nowInSecs();
  auto readFailures{0};
  for (const auto& [fruName, sensorMap] : *sensorConfig_.sensorMapList()) {
    for (const auto& [sensorName, sensor] : sensorMap) {
      SensorData sensorData{};
      sensorData.name() = sensorName;
      std::string sensorValue;
      bool sysfsFileExists =
          std::filesystem::exists(std::filesystem::path(*sensor.path()));
      if (sysfsFileExists &&
          folly::readFile(sensor.path()->c_str(), sensorValue)) {
        sensorData.value() = folly::to<float>(sensorValue);
        sensorData.timeStamp() = nowTs;
        if (sensor.compute()) {
          sensorData.value() =
              Utils::computeExpression(*sensor.compute(), *sensorData.value());
        }
        XLOG(DBG1) << fmt::format(
            "{} ({}) : {}", sensorName, *sensor.path(), *sensorData.value());
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 0);
      } else {
        XLOG(ERR) << fmt::format(
            "Could not read data for {} from path:{}, error:{}",
            sensorName,
            *sensor.path(),
            sysfsFileExists ? folly::errnoStr(errno) : "File does not exist");
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 1);
        readFailures++;
      }
      // We log 0 if there is a read failure.  If we dont log 0 on failure,
      // fb303 will pick up the last reported (on read success) value and
      // keep reporting that as the value. For 0 values, it is accurate to
      // read the value along with the kReadFailure counter. Alternative is
      // to delete this counter if there is a failure.
      fb303::fbData->setCounter(
          fmt::format(kReadValue, sensorName), sensorData.value().value_or(0));
      polledData[sensorName] = sensorData;
    }
  }
  fb303::fbData->setCounter(kTotalReadFailure, readFailures);
  fb303::fbData->setCounter(kReadTotal, polledData.size());
  fb303::fbData->setCounter(kHasReadFailure, readFailures > 0 ? 1 : 0);
  XLOG(INFO) << fmt::format(
      "Processed {} Sensors. {} Failures.", polledData.size(), readFailures);
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

} // namespace facebook::fboss::platform::sensor_service
