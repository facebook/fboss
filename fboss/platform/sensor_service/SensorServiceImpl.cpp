/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/FileUtil.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"

#include "common/fbwhoami/FbWhoAmI.h"
#include "common/time/Time.h"

namespace {
// Sensor conf file should be <platform>_sensor_config.json by default
const std::string kConfDir = "/etc/sensor_service/";
const std::string kSensorConfPostfix = "_sensor_config.json";

// The following are keys in sensor conf file
const std::string kSourceLmsensor = "lmsensor";
const std::string kSourceSysfs = "sysfs";
const std::string kSourceMock = "mock";
const std::string kSensorFieldName = "name";

const std::string kMockLmsensorJasonData =
    "/etc/sensor_service/sensors_output.json";
const std::string kLmsensorCommand = "sensors -j";
} // namespace
namespace facebook::fboss::platform::sensor_service {
using namespace facebook::fboss::platform::helpers;

void SensorServiceImpl::init() {
  // Check if conf file name is set, if not, set the default name
  if (confFileName_ == "") {
    std::string modelName = FbWhoAmI::getModelName();
    confFileName_ =
        folly::to<std::string>(kConfDir, modelName, kSensorConfPostfix);
  }

  // Clear everything before init
  sensorNameMap_.clear();
  sensorTable_.sensorMapList_ref()->clear();

  std::string sensorConfJson;
  // folly::dynamic sensorConf;

  if (folly::readFile(confFileName_.c_str(), sensorConfJson)) {
    apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
        sensorConfJson, sensorTable_);

    XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
        sensorTable_);

    if (sensorTable_.source_ref() == kSourceMock) {
      sensorSource_ = SensorSource::MOCK;
    } else if (sensorTable_.source_ref() == kSourceLmsensor) {
      sensorSource_ = SensorSource::LMSENSOR;
    } else if (sensorTable_.source_ref() == kSourceSysfs) {
      sensorSource_ = SensorSource::SYSFS;
    } else {
      throw std::runtime_error(folly::to<std::string>(
          "Invalid source in ",
          confFileName_,
          " : ",
          *sensorTable_.source_ref()));
    }

    for (auto& sensor : *sensorTable_.sensorMapList_ref()) {
      sensorNameMap_[*sensor.second.path_ref()] = sensor.first;
    }
  } else {
    throw std::runtime_error(
        "Can not find sensor config file: " + confFileName_);
  }

  for (auto& pair : *sensorTable_.sensorMapList_ref()) {
    XLOG(INFO) << pair.first << " : " << *pair.second.name_ref() << " "
               << *pair.second.path_ref() << " " << *pair.second.maxVal_ref()
               << " ";
  }

  XLOG(INFO) << "-------------------";
}

std::optional<SensorData> SensorServiceImpl::getSensorData(
    const std::string& sensorName) {
  SensorData d;
  d.name_ref() = "";

  liveDataTable_.withRLock([&](auto& table) {
    auto it = table.find(sensorName);

    if (it != table.end()) {
      d.name_ref() = it->first;
      d.value_ref() = it->second.value;
      d.timeStamp_ref() = it->second.timeStamp;
    }
  });

  return *d.name_ref() == "" ? std::nullopt : std::optional<SensorData>{d};
}

std::vector<SensorData> SensorServiceImpl::getAllSensorData() {
  std::vector<SensorData> sensorDataVec;

  liveDataTable_.withRLock([&](auto& table) {
    for (auto& pair : table) {
      SensorData d;
      d.name_ref() = pair.first;
      d.value_ref() = pair.second.value;
      d.timeStamp_ref() = pair.second.timeStamp;
      sensorDataVec.push_back(d);
    }
  });
  return sensorDataVec;
}

void SensorServiceImpl::fetchSensorData() {
  if (sensorSource_ == SensorSource::LMSENSOR) {
    int retVal = 0;
    std::string ret = execCommandUnchecked(kLmsensorCommand, retVal);

    if (retVal != 0) {
      throw std::runtime_error("Run " + kLmsensorCommand + " failed!");
    }

    parseSensorJsonData(ret);

  } else if (sensorSource_ == SensorSource::SYSFS) {
    // ToDo
    // Get sensor value via read from path (key of sensorTable_)
  } else if (sensorSource_ == SensorSource::MOCK) {
    std::string sensorDataJson;
    if (folly::readFile(kMockLmsensorJasonData.c_str(), sensorDataJson)) {
      parseSensorJsonData(sensorDataJson);
    } else {
      throw std::runtime_error(
          "Can not find sensor data json file: " + kMockLmsensorJasonData);
    }
  } else {
    throw std::runtime_error(
        "Unknow Sensor Source selected : " +
        folly::to<std::string>(static_cast<int>(sensorSource_)));
  }
}

void SensorServiceImpl::parseSensorJsonData(const std::string& strJson) {
  folly::dynamic sensorJson = folly::parseJson(strJson);

  auto dataTable = liveDataTable_.wlock();

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
              (*dataTable)[sensorNameMap_[sensorPath]].timeStamp =
                  WallClockUtil::NowInSecFast();

              XLOG(INFO) << sensorNameMap_[sensorPath] << " : "
                         << (*dataTable)[sensorNameMap_[sensorPath]].value
                         << " >>>> "
                         << (*dataTable)[sensorNameMap_[sensorPath]].timeStamp;
            }
          }
        }
      }
    }
  }
}

} // namespace facebook::fboss::platform::sensor_service
