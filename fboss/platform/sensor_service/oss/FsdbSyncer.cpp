// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/FsdbSyncer.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSyncer::getSensorServiceStatsPath() {
  return {"sensor_service"};
}
} // namespace facebook::fboss
