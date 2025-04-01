// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/SubscribableStorage.h"
#include "fboss/fsdb/oper/SubscriptionManager.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include "fboss/fsdb/server/OperPathToPublisherRoot.h"
#include "fboss/lib/ThreadHeartbeat.h"
#include "fboss/thrift_cow/gen-cpp2/patch_types.h"

#include <folly/Synchronized.h>
#include <folly/coro/AsyncScope.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>

DECLARE_int32(storage_thread_heartbeat_ms);
DECLARE_bool(serveHeartbeats);

namespace facebook::fboss::fsdb {

inline constexpr std::string_view kServeSubMs{"storage.serve_sub_ms"};
inline constexpr std::string_view kServeSubNum{"storage.serve_sub_num"};
inline constexpr std::string_view kRss{"rss"};
inline constexpr std::string_view kRegisteredSubs{"subscriptions.registered"};
inline constexpr std::string_view kPathStoreNum{"object.count.pathStores"};
inline constexpr std::string_view kPathStoreAllocs{"object.allocs.pathStores"};
inline constexpr std::string_view kPublishTimePrefix{"publish_time_ms"};
inline constexpr std::string_view kSubscribeTimePrefix{"subscribe_time_ms"};
inline constexpr std::string_view kSubscriberPrefix{"subscriber"};
inline constexpr std::string_view kSubscriptionQueueWatermark{
    "queue_watermark"};

// non-templated parts of NaivePeriodicSubscribableStorage to help with
// compilation
class NaivePeriodicSubscribableStorageBase {
 public:
  using ConcretePath = typename OperPathToPublisherRoot::Path;
  using PathIter = typename OperPathToPublisherRoot::PathIter;
  using ExtPath = typename OperPathToPublisherRoot::ExtPath;
  using ExtPathIter = typename OperPathToPublisherRoot::ExtPathIter;

  struct StorageParams {
    StorageParams(
        std::chrono::milliseconds subscriptionServeInterval =
            std::chrono::milliseconds(50),
        std::chrono::milliseconds subscriptionHeartbeatInterval =
            std::chrono::seconds(5),
        bool trackMetadata = false,
        const std::string& metricPrefix = "fsdb",
        bool convertToIDPaths = false,
        bool requireResponseOnInitialSync = false,
        bool exportPerSubscriberMetrics = false)
        : subscriptionServeInterval_(subscriptionServeInterval),
          subscriptionHeartbeatInterval_(subscriptionHeartbeatInterval),
          trackMetadata_(trackMetadata),
          metricPrefix_(metricPrefix),
          convertSubsToIDPaths_(convertToIDPaths),
          requireResponseOnInitialSync_(requireResponseOnInitialSync),
          exportPerSubscriberMetrics_(exportPerSubscriberMetrics) {}

    const std::chrono::milliseconds subscriptionServeInterval_;
    const std::chrono::milliseconds subscriptionHeartbeatInterval_;
    const bool trackMetadata_;
    const std::string& metricPrefix_;
    bool convertSubsToIDPaths_;
    const bool requireResponseOnInitialSync_;
    const bool exportPerSubscriberMetrics_;
  };

  explicit NaivePeriodicSubscribableStorageBase(StorageParams params);

  virtual ~NaivePeriodicSubscribableStorageBase() {}

  FsdbOperTreeMetadataTracker getMetadata() const;

  std::shared_ptr<ThreadHeartbeat> getThreadHeartbeat() {
    return threadHeartbeat_;
  }

  void start_impl();
  void stop_impl();

  void registerPublisher(PathIter begin, PathIter end);

  void unregisterPublisher(
      PathIter begin,
      PathIter end,
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE);

  template <typename T, typename TC>
  folly::coro::AsyncGenerator<DeltaValue<T>&&> subscribe_impl(
      SubscriptionIdentifier&& subscriber,
      PathIter begin,
      PathIter end) {
    auto sourceGen = subscribe_encoded_impl(
        std::move(subscriber), begin, end, OperProtocol::BINARY);
    return folly::coro::co_invoke(
        [&, gen = std::move(sourceGen)]() mutable
        -> folly::coro::AsyncGenerator<DeltaValue<T>&&> {
          while (auto item = co_await gen.next()) {
            auto&& encodedValue = *item;
            DeltaValue<T> typedValue;
            if (encodedValue.oldVal) {
              if (auto contents = encodedValue.oldVal->contents()) {
                typedValue.oldVal = thrift_cow::deserialize<TC, T>(
                    OperProtocol::BINARY, *contents);
              }
            }
            if (encodedValue.newVal) {
              if (auto contents = encodedValue.newVal->contents()) {
                typedValue.newVal = thrift_cow::deserialize<TC, T>(
                    OperProtocol::BINARY, *contents);
              }
            }
            co_yield std::move(typedValue);
          }
        });
  }

  folly::coro::AsyncGenerator<DeltaValue<OperState>&&> subscribe_encoded_impl(
      SubscriptionIdentifier&& subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  folly::coro::AsyncGenerator<OperDelta&&> subscribe_delta_impl(
      SubscriptionIdentifier&& subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
  subscribe_encoded_extended_impl(
      SubscriptionIdentifier&& subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  folly::coro::AsyncGenerator<std::vector<TaggedOperDelta>&&>
  subscribe_delta_extended_impl(
      SubscriptionIdentifier&& subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  folly::coro::AsyncGenerator<SubscriberMessage&&> subscribe_patch_impl(
      SubscriptionIdentifier&& subscriber,
      std::map<SubscriptionKey, RawOperPath> rawPaths,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  folly::coro::AsyncGenerator<SubscriberMessage&&>
  subscribe_patch_extended_impl(
      SubscriptionIdentifier&& subscriber,
      std::map<SubscriptionKey, ExtendedOperPath> paths,
      std::optional<SubscriptionStorageParams> subscriptionParams =
          std::nullopt);

  size_t numSubscriptions() const {
    return subMgr().numSubscriptions();
  }
  std::vector<OperSubscriberInfo> getSubscriptions() const {
    return subMgr().getSubscriptions();
  }
  size_t numPathStores() const {
    return subMgr().numPathStores();
  }
  size_t numPathStoresRecursive_Expensive() const {
    return subMgr().numPathStoresRecursive_Expensive();
  }
  uint64_t numPathStoreAllocs() const {
    return subMgr().numPathStoreAllocs();
  }
  uint64_t numPathStoreFrees() const {
    return subMgr().numPathStoreFrees();
  }

  void setConvertToIDPaths(bool convertToIDPaths) {
    params_.convertSubsToIDPaths_ = convertToIDPaths;
    subMgr().useIdPaths(convertToIDPaths);
  }

 protected:
  virtual folly::coro::Task<void> serveSubscriptions() = 0;
  virtual ConcretePath convertPath(ConcretePath&& path) const = 0;
  virtual ExtPath convertPath(const ExtPath& path) const = 0;
  virtual const SubscriptionManagerBase& subMgr() const = 0;
  virtual SubscriptionManagerBase& subMgr() = 0;

  SubscriptionMetadataServer getCurrentMetadataServer();
  void exportServeMetrics(
      std::chrono::steady_clock::time_point serveStartTime,
      SubscriptionMetadataServer& metadata,
      std::map<std::string, uint64_t>& lastServedPublisherRootUpdates) const;

  std::optional<std::string> getPublisherRoot(PathIter begin, PathIter end)
      const;
  std::optional<std::string> getPublisherRoot(
      const std::vector<ExtendedOperPath>& paths) const;

  std::optional<std::string> getPublisherRoot(
      ExtPathIter begin,
      ExtPathIter end) const;

  std::optional<std::string> getPublisherRoot(
      const std::map<SubscriptionKey, RawOperPath>& paths) const;

  std::optional<std::string> getPublisherRoot(
      const std::map<SubscriptionKey, ExtendedOperPath>& paths) const;

  void updateMetadata(
      PathIter begin,
      PathIter end,
      const OperMetadata& metadata = {});

  std::vector<ExtendedOperPath> convertExtPaths(
      const std::vector<ExtendedOperPath>& paths) const;

  folly::Synchronized<bool> running_{false};

  StorageParams params_;
  folly::Synchronized<std::unique_ptr<FsdbOperTreeMetadataTracker>>
      metadataTracker_;

  // as an optimization, for now we decide what protocol is used in patches
  // instead of letting the client choose
  const OperProtocol patchOperProtocol_{OperProtocol::COMPACT};

 private:
  folly::Synchronized<std::map<std::string, std::string>>
      registeredPublisherRoots_;
  folly::coro::CancellableAsyncScope backgroundScope_;
  std::unique_ptr<std::thread> subscriptionServingThread_;
  folly::EventBase evb_;
  std::unique_ptr<folly::ScopedEventBaseThread> heartbeatThread_;

  std::shared_ptr<ThreadHeartbeat> threadHeartbeat_;

  // metric names
  const std::string rss_{""};
  const std::string registeredSubs_{""};
  const std::string nPathStores_{""};
  const std::string nPathStoreAllocs_{""};
  const std::string serveSubMs_{""};
  const std::string serveSubNum_{""};
  // per-PublisherRoot metrics
  const std::string publishTimePrefix_;
  const std::string subscribeTimePrefix_;
  // per-subscriber metrics
  const std::string subscriberPrefix_;

  // delete copy constructors
  NaivePeriodicSubscribableStorageBase(
      NaivePeriodicSubscribableStorageBase const&) = delete;
  NaivePeriodicSubscribableStorageBase& operator=(
      NaivePeriodicSubscribableStorageBase const&) = delete;
};

} // namespace facebook::fboss::fsdb
