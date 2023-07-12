// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSensorSubscriber::getSensorDataStatsPath() {
  return {"sensor_service", "sensorData"};
}

std::vector<std::string> FsdbSensorSubscriber::getQsfpDataStatsPath() {
  return {"qsfp_service", "tcvrStats"};
}

std::vector<std::string> FsdbSensorSubscriber::getQsfpDataStatePath() {
  return {"qsfp_service", "state", "tcvrState"};
}
} // namespace facebook::fboss
