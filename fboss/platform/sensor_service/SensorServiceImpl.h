/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"
#include "folly/Synchronized.h"

namespace facebook::fboss::platform::sensor_service {

using namespace facebook::fboss::platform::sensor_config;

enum class SensorSource {
  LMSENSOR,
  SYSFS,
  MOCK,
  UNKNOWN,
};

enum class SensorType {
  FAN,
  VOLTAGE,
  CURRENT,
  TEMPERATURE,
  UNKNOWN,
};

struct SensorLiveData {
  float value;
  int64_t timeStamp;
};

class SensorServiceImpl {
 public:
  SensorServiceImpl() {
    init();
  }
  explicit SensorServiceImpl(const std::string& confFileName)
      : confFileName_{confFileName} {
    init();
  }

  std::optional<SensorData> getSensorData(const std::string& sensorName);
  std::vector<SensorData> getAllSensorData();
  void fetchSensorData();

  virtual ~SensorServiceImpl() = default;

 private:
  // Sensor config file full path
  std::string confFileName_{};

  SensorSource sensorSource_{SensorSource::LMSENSOR};

  SensorConfig sensorTable_;

  // Sensor Name map, sensor path -> sensor name
  std::unordered_map<std::string, std::string> sensorNameMap_;

  // Live sensor data table, sensor name -> sensor live data
  folly::Synchronized<std::unordered_map<std::string, struct SensorLiveData>>
      liveDataTable_;

  void init();
  void parseSensorJsonData(const std::string&);
  void getSensorDataFromPath();
};

} // namespace facebook::fboss::platform::sensor_service
