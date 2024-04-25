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

  void registerPublisher(PathIter begin, PathIter end) {
    if (!trackMetadata_) {
      return;
    }
    metadataTracker_.withWLock([&](auto& tracker) {
      CHECK(tracker);
      tracker->registerPublisherRoot(*getPublisherRoot(begin, end));
    });
  }

  void unregisterPublisher(
      PathIter begin,
      PathIter end,
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE) {
    if (!trackMetadata_) {
      return;
    }
    // Acquire subscriptions lock since we may need to
    // trim subscriptions is corresponding publishers goes
    // away. Acquiring locks in the same order subscriptionMgr,
    // metadataTracker to keep TSAN happy
    withSubMgrWLocked([&](SubscriptionManagerBase& mgr) {
      metadataTracker_.withWLock([&](auto& tracker) {
        CHECK(tracker);
        auto publisherRoot = getPublisherRoot(begin, end);
        CHECK(publisherRoot);
        tracker->unregisterPublisherRoot(*publisherRoot);
        if (!tracker->getPublisherRootMetadata(*publisherRoot)) {
          mgr.closeNoPublisherActiveSubscriptions(
              SubscriptionMetadataServer(tracker->getAllMetadata()),
              disconnectReason);
        }
      });
    });
  }

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
  folly::coro::AsyncGenerator<thrift_cow::Patch&&> subscribe_patch_impl(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol) {
    auto path = convertPath(ConcretePath(begin, end));
    auto [gen, subscription] = PatchSubscription::create(
        std::move(subscriber),
        path.begin(),
        path.end(),
        protocol,
        getPublisherRoot(path.begin(), path.end()));
    withSubMgrWLocked([subscription = std::move(subscription)](
                          SubscriptionManagerBase& mgr) mutable {
      mgr.registerSubscription(std::move(subscription));
    });
    return std::move(gen);
  }
#endif

  size_t numSubscriptions() const {
    return withSubMgrRLocked([](const SubscriptionManagerBase& mgr) {
      return mgr.numSubscriptions();
    });
  }
  std::vector<OperSubscriberInfo> getSubscriptions() const {
    return withSubMgrRLocked([](const SubscriptionManagerBase& mgr) {
      return mgr.getSubscriptions();
    });
  }
  size_t numPathStores() const {
    return withSubMgrRLocked(
        [](const SubscriptionManagerBase& mgr) { return mgr.numPathStores(); });
  }

  void setConvertToIDPaths(bool convertToIDPaths) {
    convertSubsToIDPaths_ = convertToIDPaths;
    withSubMgrWLocked([&](SubscriptionManagerBase& mgr) {
      mgr.useIdPaths(convertToIDPaths);
    });
  }

 protected:
  virtual folly::coro::Task<void> serveSubscriptions() = 0;
  virtual ConcretePath convertPath(ConcretePath&& path) const = 0;
  virtual ExtPath convertPath(const ExtPath& path) const = 0;

  /*
    We want to get access to subscription manager from
    NaivePeriodicSubscribableStorage in the form of SubscriptionManagerBase.
    Since we use a folly::Synchronized object to hold the mgr, having a getter
    to get the base class locked is not trivial. Using a little trick with a
    lambda to achieve this
  */
  virtual void withSubMgrRLockedImpl(
      folly::FunctionRef<void(const SubscriptionManagerBase&)> data) const = 0;
  virtual void withSubMgrWLockedImpl(
      folly::FunctionRef<void(SubscriptionManagerBase&)> data) = 0;

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
 private:
  //  Helper methods to get sub mgr that support lambdas with return values
  template <
      typename F,
      typename R = std::invoke_result_t<F, SubscriptionManagerBase&>>
  R withSubMgrRLocked(F&& f) const {
    if constexpr (std::is_void_v<R>) {
      withSubMgrRLockedImpl(std::forward<F>(f));
    } else {
      std::optional<R> r;
      withSubMgrRLockedImpl([&](const SubscriptionManagerBase& data) mutable {
        r.emplace(std::forward<F>(f)(data));
      });
      return std::move(r).value();
    }
  }

  template <
      typename F,
      typename R = std::invoke_result_t<F, SubscriptionManagerBase&>>
  R withSubMgrWLocked(F&& f) {
    if constexpr (std::is_void_v<R>) {
      withSubMgrWLockedImpl(std::forward<F>(f));
    } else {
      std::optional<R> r;
      withSubMgrWLockedImpl([&](SubscriptionManagerBase& data) mutable {
        r.emplace(std::forward<F>(f)(data));
      });
      return std::move(r).value();
    }
  }

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
