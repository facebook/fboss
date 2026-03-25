// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::fsdb {

/**
 * FsdbCgoPubSubWrapper provides a simplified C++ wrapper around
 * FsdbPubSubManager for use with CGO (Go<->C++ interop).
 *
 * Provides wrapper for:
 * - Subscribing to portMaps state
 * - Subscribing to any stats path
 *
 */
class FsdbCgoPubSubWrapper {
 public:
  explicit FsdbCgoPubSubWrapper(const std::string& clientId);

  ~FsdbCgoPubSubWrapper();

  // No copy/move
  FsdbCgoPubSubWrapper(const FsdbCgoPubSubWrapper&) = delete;
  FsdbCgoPubSubWrapper& operator=(const FsdbCgoPubSubWrapper&) = delete;
  FsdbCgoPubSubWrapper(FsdbCgoPubSubWrapper&&) = delete;
  FsdbCgoPubSubWrapper& operator=(FsdbCgoPubSubWrapper&&) = delete;

  void subscribeToOperState_portMaps(
      std::optional<int> serverPort = std::nullopt);

  void subscribeStatsPath(std::optional<int> serverPort = std::nullopt);

  std::unordered_map<std::string, bool> waitForStateUpdates();
  std::unordered_map<std::string, facebook::fboss::AgentStats>
  waitForStatsUpdates();

  const std::string& getClientId() const {
    return clientId_;
  }

  /**
   * Check if state subscription is active
   */
  bool hasStateSubscription() const {
    return stateSubscribed_.load();
  }

  /**
   * Check if stats subscription is active
   */
  bool hasStatsSubscription() const {
    return statsSubscribed_.load();
  }

 private:
  void enqueueState(const std::string& key, bool portOperState);
  void enqueueStats(
      const std::string& key,
      facebook::fboss::AgentStats&& stats);

  void processPortMapsUpdate(fsdb::OperState&& operState);

  const std::string clientId_;

  std::unique_ptr<FsdbPubSubManager> pubSubMgr_;

  // Bounded queue for buffering keyed STATE updates between FSDB callback
  // thread and CGO consumer thread
  folly::DSPSCQueue<
      std::tuple<std::string /*key*/, bool /*portOperState*/>,
      true /*may block*/>
      stateQueue_{100};

  // Bounded queue for buffering keyed STATS updates between FSDB callback
  // thread and CGO consumer thread
  folly::DSPSCQueue<
      std::tuple<std::string /*key*/, facebook::fboss::AgentStats /*stats*/>,
      true /*may block*/>
      statsQueue_{10};

  // Track subscription status
  std::atomic<bool> stateSubscribed_{false};
  std::atomic<bool> statsSubscribed_{false};

  // For tracking port oper state changes
  std::map<std::string, bool> portName2OperState_{};
};

} // namespace facebook::fboss::fsdb
