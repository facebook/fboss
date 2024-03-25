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
#include <unordered_map>
#include <vector>

#include <folly/Synchronized.h>

#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

DECLARE_int32(fsdb_statsStream_interval_seconds);

namespace facebook::fboss::platform::sensor_service {

using namespace facebook::fboss::platform::sensor_config;

class SensorServiceImpl {
 public:
  explicit SensorServiceImpl();
  ~SensorServiceImpl();

  std::vector<SensorData> getSensorsData(
      const std::vector<std::string>& sensorNames);
  std::map<std::string, SensorData> getAllSensorData();
  void fetchSensorData();

  FsdbSyncer* fsdbSyncer() {
    return fsdbSyncer_.get();
  }

 private:
  folly::Synchronized<std::map<std::string, SensorData>> polledData_{};
  std::unique_ptr<FsdbSyncer> fsdbSyncer_;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>>
      publishedStatsToFsdbAt_;
  SensorConfig sensorConfig_{};
};

} // namespace facebook::fboss::platform::sensor_service
