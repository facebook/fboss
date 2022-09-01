// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSensorSubscriber::getSensorDataStatsPath() {
  return {"sensor_service", "sensorData"};
}
} // namespace facebook::fboss
