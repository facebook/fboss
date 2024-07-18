// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/SubscriptionManager.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include "fboss/fsdb/server/OperPathToPublisherRoot.h"
#include "fboss/lib/ThreadHeartbeat.h"
#include "fboss/thrift_cow/gen-cpp2/patch_types.h"

#include <folly/Synchronized.h>
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

  void registerPublisher(PathIter begin, PathIter end);

  void unregisterPublisher(
      PathIter begin,
      PathIter end,
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE);

  template <typename T, typename TC>
  folly::coro::AsyncGenerator<DeltaValue<T>&&>
  subscribe_impl(SubscriberId subscriber, PathIter begin, PathIter end) {
    auto sourceGen =
        subscribe_encoded_impl(subscriber, begin, end, OperProtocol::BINARY);
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
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol);

  folly::coro::AsyncGenerator<OperDelta&&> subscribe_delta_impl(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol);

  folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
  subscribe_encoded_extended_impl(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol);

  folly::coro::AsyncGenerator<std::vector<TaggedOperDelta>&&>
  subscribe_delta_extended_impl(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol);

#ifdef ENABLE_PATCH_APIS
  folly::coro::AsyncGenerator<SubscriberMessage&&> subscribe_patch_impl(
      SubscriberId subscriber,
      std::map<SubscriptionKey, RawOperPath> rawPaths) {
    for (auto& [key, path] : rawPaths) {
      auto convertedPath = convertPath(std::move(*path.path()));
      path.path() = std::move(convertedPath);
    }
    auto root = getPublisherRoot(rawPaths);
    auto [gen, subscription] = ExtendedPatchSubscription::create(
        std::move(subscriber),
        std::move(rawPaths),
        patchOperProtocol_,
        std::move(root));
    subMgr().registerExtendedSubscription(std::move(subscription));
    return std::move(gen);
  }
#endif

  size_t numSubscriptions() const {
    return subMgr().numSubscriptions();
  }
  std::vector<OperSubscriberInfo> getSubscriptions() const {
    return subMgr().getSubscriptions();
  }
  size_t numPathStores() const {
    return subMgr().numPathStores();
  }

  void setConvertToIDPaths(bool convertToIDPaths) {
    convertSubsToIDPaths_ = convertToIDPaths;
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
      std::chrono::steady_clock::time_point serveStartTime) const;

  std::optional<std::string> getPublisherRoot(PathIter begin, PathIter end)
      const;
  std::optional<std::string> getPublisherRoot(
      const std::vector<ExtendedOperPath>& paths) const;

  std::optional<std::string> getPublisherRoot(
      ExtPathIter begin,
      ExtPathIter end) const;

  std::optional<std::string> getPublisherRoot(
      const std::map<SubscriptionKey, RawOperPath>& paths) const;

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

  // as an optimization, for now we decide what protocol is used in patches
  // instead of letting the client choose
  const OperProtocol patchOperProtocol_{OperProtocol::COMPACT};

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
