// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include "fboss/fsdb/server/OperPathToPublisherRoot.h"
#include "fboss/lib/ThreadHeartbeat.h"

#include <folly/experimental/coro/AsyncScope.h>
#include <folly/io/async/EventBase.h>

DECLARE_int32(storage_thread_heartbeat_ms);

namespace facebook::fboss::fsdb {

inline constexpr std::string_view kServeSubMs{"storage.serve_sub_ms"};
inline constexpr std::string_view kServeSubNum{"storage.serve_sub_num"};
inline constexpr std::string_view kRss{"rss"};

// non-templated parts of NaivePeriodicSubscribableStorage to help with
// compilation
class NaivePeriodicSubscribableStorageBase {
 public:
  using ConcretePath = typename OperPathToPublisherRoot::Path;
  using PathIter = typename OperPathToPublisherRoot::PathIter;
  using ExtPath = typename OperPathToPublisherRoot::ExtPath;
  using ExtPathIter = typename OperPathToPublisherRoot::ExtPathIter;

  NaivePeriodicSubscribableStorageBase(
      std::chrono::milliseconds subscriptionServeInterval,
      std::chrono::milliseconds subscriptionHeartbeatInterval,
      bool trackMetadata,
      const std::string& metricPrefix,
      bool convertToIDPaths);

  virtual ~NaivePeriodicSubscribableStorageBase() {}

  FsdbOperTreeMetadataTracker getMetadata() const;

  std::shared_ptr<ThreadHeartbeat> getThreadHeartbeat() {
    return threadHeartbeat_;
  }

  void start_impl();
  void stop_impl();

 protected:
  virtual folly::coro::Task<void> serveSubscriptions() = 0;
  virtual ConcretePath convertPath(ConcretePath&& path) const = 0;
  virtual ExtPath convertPath(const ExtPath& path) const = 0;

  SubscriptionMetadataServer getCurrentMetadataServer();
  void exportServeMetrics(
      std::chrono::steady_clock::time_point serveStartTime) const;

  std::optional<std::string> getPublisherRoot(PathIter begin, PathIter end)
      const;
  std::optional<std::string> getPublisherRoot(
      const std::vector<ExtendedOperPath>& paths) const;

  std::optional<std::string> getPublisherRoot(
      ExtPathIter begin,
      ExtPathIter end) const;

  void updateMetadata(
      PathIter begin,
      PathIter end,
      const OperMetadata& metadata = {});

  std::vector<ExtendedOperPath> convertExtPaths(
      const std::vector<ExtendedOperPath>& paths) const;

  folly::Synchronized<bool> running_{false};

  const std::chrono::milliseconds subscriptionServeInterval_;
  const std::chrono::milliseconds subscriptionHeartbeatInterval_;
  folly::Synchronized<std::unique_ptr<FsdbOperTreeMetadataTracker>>
      metadataTracker_;
  const bool trackMetadata_{false};

  bool convertSubsToIDPaths_{false};

  std::chrono::steady_clock::time_point lastHeartbeatTime_;

 private:
  folly::coro::CancellableAsyncScope backgroundScope_;
  std::unique_ptr<std::thread> subscriptionServingThread_;
  folly::EventBase evb_;

  std::shared_ptr<ThreadHeartbeat> threadHeartbeat_;

  // metric names
  const std::string rss_{""};
  const std::string serveSubMs_{""};
  const std::string serveSubNum_{""};

  // delete copy constructors
  NaivePeriodicSubscribableStorageBase(
      NaivePeriodicSubscribableStorageBase const&) = delete;
  NaivePeriodicSubscribableStorageBase& operator=(
      NaivePeriodicSubscribableStorageBase const&) = delete;
};

} // namespace facebook::fboss::fsdb
