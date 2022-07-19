// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/platform/sensor_service/gen-cpp2/sensor_service_stats_types.h"

#include <memory>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}
class FsdbSyncer {
 public:
  FsdbSyncer();
  ~FsdbSyncer();
  void statsUpdated(const stats::SensorServiceStats& stats);

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

  void stop();

  // Paths
  static std::vector<std::string> getSensorServiceStatsPath();

 private:
  void fsdbStatPublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);

  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> readyForStatPublishing_{false};
};

} // namespace facebook::fboss
