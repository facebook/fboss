// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/CowSubscriptionManager.h>
#include <fboss/fsdb/oper/DeltaValue.h>
#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorageBase.h>
#include <fboss/fsdb/oper/SubscribableStorage.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/storage/Storage.h>

#include <folly/Expected.h>
#include <folly/experimental/coro/Sleep.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <utility>

/*
  Patch apis require the need for a couple new visitor types which end up
  increasing our compile time by quite a bit. For now using a preproccessor flag
  ENABLE_PATCH_APIS to disable this for NSDB
*/
#ifdef ENABLE_PATCH_APIS
#include <fboss/fsdb/oper/PathConverter.h>
#endif

namespace facebook::fboss::fsdb {

DECLARE_bool(serveHeartbeats);

template <typename Storage, typename SubscribeManager>
class NaivePeriodicSubscribableStorage
    : public NaivePeriodicSubscribableStorageBase,
      public SubscribableStorage<
          typename Storage::RootT,
          NaivePeriodicSubscribableStorage<Storage, SubscribeManager>> {
 public:
  // TODO: more flexibility here for all forward iterator types
  using RootT = typename Storage::RootT;
  using ConcretePath = typename Storage::ConcretePath;
  using PathIter = typename Storage::PathIter;
  using ExtPath = typename Storage::ExtPath;
  using ExtPathIter = typename Storage::ExtPathIter;

  template <typename T>
  using Result = typename Storage::template Result<T>;

  using Self = NaivePeriodicSubscribableStorage<Storage, SubscribeManager>;
  using Base = SubscribableStorage<RootT, Self>;

  explicit NaivePeriodicSubscribableStorage(
      const RootT& initialState,
      std::chrono::milliseconds subscriptionServeInterval =
          std::chrono::milliseconds(50),
      std::chrono::milliseconds subscriptionHeartbeatInterval =
          std::chrono::seconds(5),
      bool trackMetadata = false,
      const std::string& metricPrefix = "fsdb",
      bool convertToIDPaths = false,
      bool requireResponseOnInitialSync = false)
      : NaivePeriodicSubscribableStorageBase(
            subscriptionServeInterval,
            subscriptionHeartbeatInterval,
            trackMetadata,
            metricPrefix,
            convertToIDPaths),
        currentState_(std::in_place_t{}, initialState),
        lastPublishedState_(*currentState_.rlock()) {
    subscriptions_.wlock()->setRequireResponseOnInitialSync(
        requireResponseOnInitialSync);
#ifdef ENABLE_PATCH_APIS
    subscriptions_.wlock()->useIdPaths(convertToIDPaths);
#endif
    auto currentState = currentState_.wlock();
    currentState->publish();
  }

  ~NaivePeriodicSubscribableStorage() {
    stop();
  }

  using NaivePeriodicSubscribableStorageBase::start_impl;
  using NaivePeriodicSubscribableStorageBase::stop_impl;

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
    auto subscriptions = subscriptions_.wlock();
    metadataTracker_.withWLock([&](auto& tracker) {
      CHECK(tracker);
      auto publisherRoot = getPublisherRoot(begin, end);
      CHECK(publisherRoot);
      tracker->unregisterPublisherRoot(*publisherRoot);
      if (!tracker->getPublisherRootMetadata(*publisherRoot)) {
        subscriptions->closeNoPublisherActiveSubscriptions(
            SubscriptionMetadataServer(tracker->getAllMetadata()),
            disconnectReason);
      }
    });
  }

  using Base::add;
  using Base::get;
  using Base::get_encoded;
  using Base::get_encoded_extended;
  using Base::set;
  using Base::set_encoded;
  using Base::start;
  using Base::stop;
  using Base::subscribe;
  using Base::subscribe_delta;
  using Base::subscribe_delta_extended;
  using Base::subscribe_encoded;
  using Base::subscribe_encoded_extended;

  template <typename T>
  Result<T> get_impl(PathIter begin, PathIter end) const {
    auto state = currentState_.rlock();
    return state->template get<T>(begin, end);
  }

  Result<OperState>
  get_encoded_impl(PathIter begin, PathIter end, OperProtocol protocol) const {
    auto state = currentState_.rlock();
    auto result = state->get_encoded(begin, end, protocol);
    if (result.hasValue() && trackMetadata_) {
      metadataTracker_.withRLock([&](auto& tracker) {
        CHECK(tracker);
        auto metadata =
            tracker->getPublisherRootMetadata(*getPublisherRoot(begin, end));
        if (metadata && *metadata->operMetadata.lastConfirmedAt() > 0) {
          result.value().metadata() = metadata->operMetadata;
        } else {
          throw Utils::createFsdbException(
              FsdbErrorCode::PUBLISHER_NOT_READY, "Publisher not ready");
        }
      });
    }
    return result;
  }

  Result<std::vector<TaggedOperState>> get_encoded_extended_impl(
      ExtPathIter begin,
      ExtPathIter end,
      OperProtocol protocol) const {
    auto state = currentState_.rlock();
    auto result = state->get_encoded_extended(begin, end, protocol);
    if (result.hasValue() && trackMetadata_) {
      metadataTracker_.withRLock([&](auto& tracker) {
        CHECK(tracker);
        auto metadata =
            tracker->getPublisherRootMetadata(*getPublisherRoot(begin, end));
        if (metadata && *metadata->operMetadata.lastConfirmedAt() > 0) {
          for (auto& state : result.value()) {
            state.state()->metadata() = metadata->operMetadata;
          }
        } else {
          throw Utils::createFsdbException(
              FsdbErrorCode::PUBLISHER_NOT_READY, "Publisher not ready");
        }
      });
    }
    return result;
  }

  template <typename T>
  std::optional<StorageError>
  set_impl(PathIter begin, PathIter end, T&& value) {
    auto state = currentState_.wlock();
    updateMetadata(begin, end);
    return state->set(begin, end, std::forward<T>(value));
  }

  std::optional<StorageError>
  set_encoded_impl(PathIter begin, PathIter end, const OperState& value) {
    auto state = currentState_.wlock();
    auto metadata = value.metadata() ? *value.metadata() : OperMetadata();
    updateMetadata(begin, end, metadata);
    return state->set_encoded(begin, end, value);
  }

  template <typename T>
  std::optional<StorageError>
  add_impl(PathIter begin, PathIter end, T&& value) {
    auto state = currentState_.wlock();
    updateMetadata(begin, end);
    return state->add(begin, end, std::forward<T>(value));
  }

  void remove_impl(PathIter begin, PathIter end) {
    auto state = currentState_.wlock();
    updateMetadata(begin, end);
    state->remove(begin, end);
  }

#ifdef ENABLE_PATCH_APIS
  std::optional<StorageError> patch_impl(thrift_cow::Patch&& patch) {
    if (patch.patch()->getType() == thrift_cow::PatchNode::Type::__EMPTY__) {
      return std::nullopt;
    }
    auto& path = *patch.basePath();
    // TODO: include metadata in patch
    auto metadata = OperMetadata();
    auto state = currentState_.wlock();
    updateMetadata(path.begin(), path.end(), metadata);
    return state->patch(std::move(patch));
  }
#endif

  std::optional<StorageError> patch_impl(const fsdb::OperDelta& delta) {
    if (!delta.changes()->size()) {
      return std::nullopt;
    }
    // Pick the publisher root path from first unit.
    // TODO - have caller to patch send the path like
    // we do for oper state
    auto& path = *delta.changes()->begin()->path()->raw();
    auto state = currentState_.wlock();
    auto metadata = delta.metadata() ? *delta.metadata() : OperMetadata();
    updateMetadata(path.begin(), path.end(), metadata);
    return state->patch(delta);
  }

  std::optional<StorageError> patch_impl(
      const fsdb::TaggedOperState& operState) {
    auto& path = *operState.path()->path();
    auto state = currentState_.wlock();
    auto metadata = operState.state()->metadata()
        ? *operState.state()->metadata()
        : OperMetadata();
    updateMetadata(path.begin(), path.end(), metadata);
    return state->patch(operState);
  }

  template <typename T, typename TC>
  folly::coro::AsyncGenerator<DeltaValue<T>&&>
  subscribe_impl(SubscriberId subscriber, PathIter begin, PathIter end) {
    auto sourceGen = subscribe_internal<OperState>(
        subscriber, begin, end, OperProtocol::BINARY);
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
      OperProtocol protocol) {
    return subscribe_internal<OperState>(subscriber, begin, end, protocol);
  }

  folly::coro::AsyncGenerator<OperDelta&&> subscribe_delta_impl(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol) {
    auto path = convertPath(ConcretePath(begin, end));
    auto [gen, subscription] = DeltaSubscription::create(
        std::move(subscriber),
        path.begin(),
        path.end(),
        protocol,
        getPublisherRoot(path.begin(), path.end()));
    auto subscriptions = subscriptions_.wlock();
    subscriptions->registerSubscription(std::move(subscription));
    return std::move(gen);
  }

  folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
  subscribe_encoded_extended_impl(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol) {
    paths = convertExtPaths(paths);
    auto publisherRoot = getPublisherRoot(paths);
    auto [gen, subscription] = ExtendedPathSubscription::create(
        std::move(subscriber),
        std::move(paths),
        std::move(publisherRoot),
        protocol);
    auto subscriptions = subscriptions_.wlock();
    subscriptions->registerExtendedSubscription(std::move(subscription));
    return std::move(gen);
  }

  folly::coro::AsyncGenerator<std::vector<TaggedOperDelta>&&>
  subscribe_delta_extended_impl(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      OperProtocol protocol) {
    paths = convertExtPaths(paths);
    auto publisherRoot = getPublisherRoot(paths);
    auto [gen, subscription] = ExtendedDeltaSubscription::create(
        std::move(subscriber),
        std::move(paths),
        std::move(publisherRoot),
        protocol);
    auto subscriptions = subscriptions_.wlock();
    subscriptions->registerExtendedSubscription(std::move(subscription));
    return std::move(gen);
  }

  folly::coro::Task<void> serveSubscriptions() override {
    while (true) {
      auto start = std::chrono::steady_clock::now();

      if (auto runningLocked = running_.rlock(); !*runningLocked) {
        break;
      }

      {
        auto currentState = currentState_.rlock();
        auto lastState = lastPublishedState_.wlock();
        auto subscriptions = subscriptions_.wlock();

        /*
         * Grab a copy of metadata while holding current state
         * lock. This way we are guaranteed to get metadata
         * corresponding to currentState
         */
        SubscriptionMetadataServer metadataServer = getCurrentMetadataServer();

        subscriptions->pruneCancelledSubscriptions();

        if (*lastState != *currentState) {
          auto newState = *currentState;
          subscriptions->publishAndAddPaths(newState);
          subscriptions->serveSubscriptions(
              *lastState, newState, metadataServer);
          subscriptions->pruneDeletedPaths(*lastState, newState);
          *lastState = std::move(newState);
        }
        // Serve new subscriptions after serving existing subscriptions.
        // New subscriptions will get a full object dump on first sync.
        // If we serve them before the loop above, we have to be careful
        // to not serve them again in the loop above. So just move to serve
        // after. Post the initial sync, these new subscriptions will be
        // pruned from initialSyncNeeded list and will get served on
        // changes only
        subscriptions->initialSyncForNewSubscriptions(
            *currentState, metadataServer);
      }

      if (FLAGS_serveHeartbeats &&
          start - lastHeartbeatTime >= subscriptionHeartbeatInterval_) {
        subscriptions_.wlock()->serveHeartbeat();
        lastHeartbeatTime = start;
      }

      exportServeMetrics(start);

      co_await folly::coro::sleep(subscriptionServeInterval_);
    }
  }

  size_t numSubscriptions() const {
    return subscriptions_.rlock()->numSubscriptions();
  }
  std::vector<OperSubscriberInfo> getSubscriptions() const {
    return subscriptions_.rlock()->getSubscriptions();
  }
  size_t numPathStores() const {
    return subscriptions_.rlock()->numPathStores();
  }
  /*
   * Expensive API to copy current root. To be used only
   * in tests
   */
  RootT currentStateExpensive() const {
    return currentState_.rlock()->root()->toThrift();
  }

  OperState publishedStateEncoded(OperProtocol protocol) {
    auto lastState = lastPublishedState_.rlock();
    std::vector<std::string> rootPath;
    return *lastState->get_encoded(rootPath.begin(), rootPath.end(), protocol);
  }

  void setConvertToIDPaths(bool convertToIDPaths) {
    convertSubsToIDPaths_ = convertToIDPaths;
    subscriptions_.wlock()->useIdPaths(convertToIDPaths);
  }

 private:
  template <typename TargetType, typename... Args>
  folly::coro::AsyncGenerator<DeltaValue<TargetType>&&> subscribe_internal(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      Args&&... args) {
    auto path = convertPath(ConcretePath(begin, end));
    auto [gen, subscription] = PathSubscription<DeltaValue<TargetType>>::create(
        std::move(subscriber),
        path.begin(),
        path.end(),
        getPublisherRoot(path.begin(), path.end()),
        std::forward<Args>(args)...);
    auto subscriptions = subscriptions_.wlock();
    subscriptions->registerSubscription(std::move(subscription));
    return std::move(gen);
  }

  ConcretePath convertPath(ConcretePath&& path) const override {
#ifdef ENABLE_PATCH_APIS
    return convertSubsToIDPaths_
        ? PathConverter<RootT>::pathToIdTokens(std::move(path))
        : path;
#else
    return path;
#endif
  }

  ExtPath convertPath(const ExtPath& path) const override {
#ifdef ENABLE_PATCH_APIS
    return convertSubsToIDPaths_ ? PathConverter<RootT>::extPathToIdTokens(path)
                                 : path;
#else
    return path;
#endif
  }

  folly::Synchronized<Storage> currentState_;
  folly::Synchronized<Storage> lastPublishedState_;

  folly::Synchronized<SubscribeManager> subscriptions_;
};

template <typename Root>
using NaivePeriodicSubscribableCowStorage = NaivePeriodicSubscribableStorage<
    CowStorage<Root>,
    CowSubscriptionManager<CowStorage<Root>>>;

} // namespace facebook::fboss::fsdb
