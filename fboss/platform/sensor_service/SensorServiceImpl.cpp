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
#include <folly/dynamic.h>
#include <folly/json.h>
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

DEFINE_string(
    mock_lmsensor_json_data,
    "/etc/sensor_service/sensors_output.json",
    "File to store the mock Lm Sensor JSON data");

namespace {

// The following are keys in sensor conf file
const std::string kSourceLmsensor = "lmsensor";
const std::string kSourceSysfs = "sysfs";
const std::string kSourceMock = "mock";
const std::string kSensorFieldName = "name";

const std::string kLmsensorCommand = "sensors -j";

auto constexpr kReadFailure = "sensor_read.{}.failure";
auto constexpr kTotalReadFailure = "sensor_read.total.failures";
auto constexpr kHasReadFailure = "sensor_read.has.failures";
} // namespace
namespace facebook::fboss::platform::sensor_service {

SensorServiceImpl::SensorServiceImpl(std::string confFileName)
    : confFileName_(std::move(confFileName)) {
  std::string sensorConfJson;
  // Check if conf file name is set, if not, set the default name
  if (confFileName_.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    sensorConfJson = ConfigLib().getSensorServiceConfig();
  } else {
    XLOG(INFO) << "Using config file: " << confFileName_;
    if (!folly::readFile(confFileName_.c_str(), sensorConfJson)) {
      throw std::runtime_error(
          "Can not find sensor config file: " + confFileName_);
    }
  }

  XLOG(DBG2) << "Read sensor config: " << sensorConfJson;

  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorConfJson, sensorTable_);

  if (sensorTable_.source() == kSourceMock) {
    sensorSource_ = SensorSource::MOCK;
  } else if (sensorTable_.source() == kSourceLmsensor) {
    sensorSource_ = SensorSource::LMSENSOR;
  } else if (sensorTable_.source() == kSourceSysfs) {
    sensorSource_ = SensorSource::SYSFS;
  } else {
    throw std::runtime_error(folly::to<std::string>(
        "Invalid source in ", confFileName_, " : ", *sensorTable_.source()));
  }

  liveDataTable_.withWLock([&](auto& table) {
    for (const auto& [fruName, sensorMap] : *sensorTable_.sensorMapList()) {
      for (const auto& [sensorName, sensor] : sensorMap) {
        std::string path = *sensor.path();
        if (std::filesystem::exists(std::filesystem::path(path))) {
          table[sensorName].path = path;
          sensorNameMap_[path] = sensorName;
        }
        table[sensorName].fru = fruName;
        if (sensor.compute().has_value()) {
          table[sensorName].compute = *sensor.compute();
        }
        table[sensorName].thresholds = *sensor.thresholds();

        XLOG(INFO) << sensorName << "; path = " << table[sensorName].path
                   << "; compute = " << table[sensorName].compute
                   << "; fru = " << table[sensorName].fru;
      }
    }
  });

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
  liveDataTable_.withRLock([&](auto& table) {
    for (const auto& sensorName : sensorNames) {
      auto it = table.find(sensorName);
      if (it == table.end()) {
        continue;
      }
      SensorData d;
      d.name() = it->first;
      d.value().from_optional(it->second.value);
      d.timeStamp().from_optional(it->second.timeStamp);
      sensorDataVec.push_back(d);
    }
  });
  return sensorDataVec;
}

std::map<std::string, SensorData> SensorServiceImpl::getAllSensorData() {
  std::map<std::string, SensorData> sensorDataMap;

  liveDataTable_.withRLock([&](auto& table) {
    for (auto& pair : table) {
      SensorData d;
      d.name() = pair.first;
      d.value().from_optional(pair.second.value);
      d.timeStamp().from_optional(pair.second.timeStamp);
      sensorDataMap[pair.first] = d;
    }
  });
  return sensorDataMap;
}

void SensorServiceImpl::fetchSensorData() {
  SCOPE_EXIT {
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
  };
  if (sensorSource_ == SensorSource::LMSENSOR) {
    auto [exitStatus, standardOut] =
        PlatformUtils().execCommand(kLmsensorCommand);
    if (exitStatus != 0) {
      throw std::runtime_error("Run " + kLmsensorCommand + " failed!");
    }
    parseSensorJsonData(standardOut);
  } else if (sensorSource_ == SensorSource::SYSFS) {
    getSensorDataFromPath();
  } else if (sensorSource_ == SensorSource::MOCK) {
    std::string sensorDataJson;
    if (folly::readFile(
            FLAGS_mock_lmsensor_json_data.c_str(), sensorDataJson)) {
      parseSensorJsonData(sensorDataJson);
    } else {
      throw std::runtime_error(
          "Can not find sensor data json file: " +
          FLAGS_mock_lmsensor_json_data);
    }
  } else {
    throw std::runtime_error(
        "Unknown Sensor Source selected : " +
        folly::to<std::string>(static_cast<int>(sensorSource_)));
  }
}

void SensorServiceImpl::getSensorDataFromPath() {
  liveDataTable_.withWLock([&](auto& liveDataTable) {
    auto now = Utils::nowInSecs();
    auto readFailures{0};
    for (auto& [sensorName, sensorLiveData] : liveDataTable) {
      std::string sensorValue;
      if (folly::readFile(sensorLiveData.path.c_str(), sensorValue)) {
        sensorLiveData.value = folly::to<float>(sensorValue);
        sensorLiveData.timeStamp = now;
        if (!sensorLiveData.compute.empty()) {
          sensorLiveData.value = Utils::computeExpression(
              sensorLiveData.compute, *sensorLiveData.value);
        }
        XLOG(INFO) << fmt::format(
            "{} ({}) : {}",
            sensorName,
            sensorLiveData.path,
            *sensorLiveData.value);
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 0);
      } else {
        sensorLiveData.value = std::nullopt;
        sensorLiveData.timeStamp = std::nullopt;
        XLOG(INFO) << fmt::format(
            "Could not read data for {} from {}",
            sensorName,
            sensorLiveData.path);
        fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 1);
        readFailures++;
      }
    }
    fb303::fbData->setCounter(kTotalReadFailure, readFailures);
    fb303::fbData->setCounter(kHasReadFailure, readFailures > 0 ? 1 : 0);
  });
}

void SensorServiceImpl::parseSensorJsonData(const std::string& strJson) {
  folly::dynamic sensorJson = folly::parseJson(strJson);

  auto dataTable = liveDataTable_.wlock();

  auto now = Utils::nowInSecs();
  for (auto& firstPair : sensorJson.items()) {
    // Key is pair.first, value is pair.second
    if (firstPair.second.isObject()) {
      for (auto& secondPair : firstPair.second.items()) {
        std::string sensorPath = folly::to<std::string>(
            firstPair.first.asString(), ":", secondPair.first.asString());
        // Only check sensor data that the name is in the configuration file
        if (secondPair.second.isObject() &&
            (sensorNameMap_.count(sensorPath) != 0)) {
          // Get value only for now
          for (auto& thirdPair : secondPair.second.items()) {
            if (thirdPair.first.asString().find("_input") !=
                std::string::npos) {
              (*dataTable)[sensorNameMap_[sensorPath]].value =
                  folly::to<float>(thirdPair.second.asString());
              (*dataTable)[sensorNameMap_[sensorPath]].timeStamp = now;

              XLOG(INFO) << sensorNameMap_[sensorPath] << " : "
                         << *(*dataTable)[sensorNameMap_[sensorPath]].value
                         << " >>>> "
                         << *(*dataTable)[sensorNameMap_[sensorPath]].timeStamp;
            }
          }
        }
      }
    }
  }
}

} // namespace facebook::fboss::platform::sensor_service
