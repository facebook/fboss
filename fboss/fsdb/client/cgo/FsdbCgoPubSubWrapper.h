// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/cgo/fsdb_cgo_api.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/FBString.h>
#include <folly/concurrency/DynamicBoundedQueue.h>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace facebook::fboss::fsdb {

// extern-C wrapper around FsdbPubSubManager for CGO consumers.
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
      std::optional<int> serverPort = std::nullopt,
      const std::optional<std::string>& host = std::nullopt);

  void subscribeStatsPath(
      const std::vector<std::string>& path,
      std::optional<int> serverPort = std::nullopt,
      const std::optional<std::string>& host = std::nullopt);

  void subscribeStatePath(
      const std::vector<std::string>& path,
      std::optional<int> serverPort = std::nullopt,
      const std::optional<std::string>& host = std::nullopt);

  // Synchronous one-shot GET of agent/switchState/portMaps; returns every port
  // as (portName, portId, portOperState). No subscription required.
  std::vector<std::tuple<std::string, int32_t, bool>> getPortSnapshot(
      std::optional<int> serverPort = std::nullopt,
      const std::optional<std::string>& host = std::nullopt);

  // Blocks for >=1 update, then non-blocking-drains up to maxCount.
  // Throws if no subscription. Empty on shutdown.
  std::vector<std::tuple<std::string, int32_t, bool>> waitForStateUpdates(
      int maxCount = std::numeric_limits<int>::max());

  std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
  waitForStatsUpdates(int maxCount = std::numeric_limits<int>::max());

  std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
  waitForStatePathUpdates(int maxCount = std::numeric_limits<int>::max());

  // Wakes any in-flight waitFor* within ~kPollInterval. SPSC-safe (flag only).
  void shutdown() noexcept {
    shuttingDown_.store(true, std::memory_order_release);
  }

  const std::string& getClientId() const {
    return clientId_;
  }

  bool hasStateSubscription() const {
    return stateSubscribed_.load();
  }

  bool hasStatsSubscription() const {
    return statsSubscribed_.load();
  }

  bool hasStatePathSubscription() const {
    return statePathSubscribed_.load();
  }

  // Latest portMaps connection state (a FsdbConnectionState value).
  int32_t getConnectionState() const {
    return portMapsConnState_.load(std::memory_order_acquire);
  }

  // Above max port count so a full initial snapshot buffers without dropping.
  static constexpr size_t kStateQueueCapacity = 1024;

  // Public so extern-C wrappers can hold borrowed pointers across calls.
  std::vector<std::tuple<std::string, int32_t, bool>> lastStateUpdates_;
  std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
      lastStatsUpdates_;
  std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
      lastStatePathUpdates_;
  std::vector<std::tuple<std::string, int32_t, bool>> lastSnapshot_;

 private:
  void enqueueState(const std::string& key, int32_t portId, bool portOperState);
  void enqueueStats(
      const std::string& key,
      folly::fbstring&& contents,
      int32_t protocol);
  void enqueueStatePath(
      const std::string& key,
      folly::fbstring&& contents,
      int32_t protocol);

  void processPortMapsUpdate(fsdb::OperState&& operState);

  const std::string clientId_;

  std::unique_ptr<FsdbPubSubManager> pubSubMgr_;

  // Bounded queue for buffering keyed STATE updates between FSDB callback
  // thread and CGO consumer thread
  folly::DSPSCQueue<
      std::tuple<
          std::string /*key*/,
          int32_t /*portId*/,
          bool /*portOperState*/>,
      true /*may block*/>
      stateQueue_{kStateQueueCapacity};

  // fbstring avoids a toStdString() copy on the producer side.
  folly::DSPSCQueue<
      std::tuple<
          std::string /*key*/,
          folly::fbstring /*contents*/,
          int32_t /*protocol*/>,
      true /*may block*/>
      statsQueue_{100};

  folly::DSPSCQueue<
      std::tuple<
          std::string /*key*/,
          folly::fbstring /*contents*/,
          int32_t /*protocol*/>,
      true /*may block*/>
      statePathQueue_{100};

  std::atomic<bool> stateSubscribed_{false};
  std::atomic<bool> statsSubscribed_{false};
  std::atomic<bool> statePathSubscribed_{false};
  std::atomic<bool> shuttingDown_{false};
  std::atomic<int32_t> portMapsConnState_{FSDB_CONNECTION_DISCONNECTED};

  std::map<std::string, bool> portName2OperState_{};
};

} // namespace facebook::fboss::fsdb
