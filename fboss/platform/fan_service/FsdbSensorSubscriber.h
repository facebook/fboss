// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

#include <memory>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}
class FsdbSensorSubscriber {
 public:
  explicit FsdbSensorSubscriber(fsdb::FsdbPubSubManager* pubSubMgr)
      : fsdbPubSubMgr_(pubSubMgr) {}

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_;
  }

  void subscribeToSensorServiceStat(
      folly::Synchronized<
          std::map<std::string, fboss::platform::sensor_service::SensorData>>&
          storage);

  // Paths
  static std::vector<std::string> getSensorDataStatsPath();

  uint64_t getLastUpdatedTime() const {
    return lastUpdatedTime.load();
  }

 private:
  template <typename T>
  void subscribeToStat(
      std::vector<std::string> path,
      folly::Synchronized<T>& storage);
  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
  std::atomic<uint64_t> lastUpdatedTime{0};
};

} // namespace facebook::fboss
