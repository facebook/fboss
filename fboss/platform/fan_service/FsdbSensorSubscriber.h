// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_types.h"

#include <cstdint>
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
  void subscribeToQsfpServiceState(
      folly::Synchronized<std::map<int32_t, TcvrState>>& storage);
  void subscribeToQsfpServiceStat(
      folly::Synchronized<std::map<int32_t, TcvrStats>>& storage);

  // Paths
  static std::vector<std::string> getSensorDataStatsPath();
  static std::vector<std::string> getQsfpDataStatsPath();
  static std::vector<std::string> getQsfpDataStatePath();

  uint64_t getSensorStatsLastUpdatedTime() const {
    return sensorStatsLastUpdatedTime.load();
  }

 private:
  template <typename T>
  void subscribeToStatsOrState(
      std::vector<std::string> path,
      folly::Synchronized<T>& storage,
      bool stats);
  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
  std::atomic<uint64_t> sensorStatsLastUpdatedTime{0};
};

} // namespace facebook::fboss
