/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

namespace facebook::fboss::fsdb {

template <typename Storage, typename SubscribeManager>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    NaivePeriodicSubscribableStorage(
        const RootT& initialState,
        StorageParams params)
    : NaivePeriodicSubscribableStorageBase(params),
      currentState_(std::in_place, initialState),
      lastPublishedState_(*currentState_.rlock()),
      subscriptions_(patchOperProtocol_, params.requireResponseOnInitialSync_) {
  subscriptions_.useIdPaths(params.convertSubsToIDPaths_);
  auto currentState = currentState_.wlock();
  currentState->publish();
}

template <typename Storage, typename SubscribeManager>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    ~NaivePeriodicSubscribableStorage() {
  stop();
}

template <typename Storage, typename SubscribeManager>
typename NaivePeriodicSubscribableStorage<
    Storage,
    SubscribeManager>::template Result<OperState>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::get_encoded_impl(
    PathIter begin,
    PathIter end,
    OperProtocol protocol) const {
  Result<OperState> result = folly::makeUnexpected(
      StorageError(StorageError::Code::INVALID_PATH, "Unknown"));
  if (params_.serveGetRequestsWithLastPublishedState_) {
    auto state = Storage(*lastPublishedState_.rlock());
    result = state.get_encoded(begin, end, protocol);
  } else {
    // hold rlock on current state to avoid racing with writers
    auto currentState = currentState_.rlock();
    result = currentState->get_encoded(begin, end, protocol);
  }
  if (result.hasValue() && params_.trackMetadata_) {
    auto publisherRoot = getPublisherRoot(begin, end);
    metadataTracker_.withRLock([&](auto& tracker) {
      CHECK(tracker);
      auto metadata = tracker->getPublisherRootMetadata(*publisherRoot);
      if (metadata && *metadata->operMetadata.lastConfirmedAt() > 0) {
        result.value().metadata() = metadata->operMetadata;
        result.value().metadata()->lastServedAt() =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
      } else {
        throw Utils::createFsdbException(
            FsdbErrorCode::PUBLISHER_NOT_READY,
            fmt::format("Publisher not ready for root: {}", *publisherRoot));
      }
    });
  }
  return result;
}

template <typename Storage, typename SubscribeManager>
typename NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    template Result<std::vector<TaggedOperState>>
    NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
        get_encoded_extended_impl(
            ExtPathIter begin,
            ExtPathIter end,
            OperProtocol protocol) const {
  Result<std::vector<TaggedOperState>> result = folly::makeUnexpected(
      StorageError(StorageError::Code::INVALID_PATH, "Unknown"));
  if (params_.serveGetRequestsWithLastPublishedState_) {
    auto state = Storage(*lastPublishedState_.rlock());
    result = state.get_encoded_extended(begin, end, protocol);
  } else {
    // hold rlock on current state to avoid racing with writers
    auto currentState = currentState_.rlock();
    result = currentState->get_encoded_extended(begin, end, protocol);
  }
  if (result.hasValue() && params_.trackMetadata_) {
    auto publisherRoot = getPublisherRoot(begin, end);
    metadataTracker_.withRLock([&](auto& tracker) {
      CHECK(tracker);
      auto metadata = tracker->getPublisherRootMetadata(*publisherRoot);
      if (metadata && *metadata->operMetadata.lastConfirmedAt() > 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
        for (auto& state : result.value()) {
          state.state()->metadata() = metadata->operMetadata;
          state.state()->metadata()->lastServedAt() = now;
        }
      } else {
        throw Utils::createFsdbException(
            FsdbErrorCode::PUBLISHER_NOT_READY,
            fmt::format("Publisher not ready for root: {}", *publisherRoot));
      }
    });
  }
  return result;
}

template <typename Storage, typename SubscribeManager>
std::optional<StorageError>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::set_encoded_impl(
    PathIter begin,
    PathIter end,
    const OperState& value) {
  auto state = currentState_.wlock();
  auto metadata = value.metadata() ? *value.metadata() : OperMetadata();
  updateMetadata(begin, end, metadata);
  return state->set_encoded(begin, end, value);
}

template <typename Storage, typename SubscribeManager>
void NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::remove_impl(
    PathIter begin,
    PathIter end) {
  auto state = currentState_.wlock();
  updateMetadata(begin, end);
  state->remove(begin, end);
}

template <typename Storage, typename SubscribeManager>
std::optional<StorageError>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::patch_impl(
    Patch&& patch) {
  if (patch.patch()->getType() == thrift_cow::PatchNode::Type::__EMPTY__) {
    XLOG(DBG3) << "Patch is empty, nothing to do";
    return StorageError(StorageError::Code::TYPE_ERROR, "Empty patch");
  }
  auto& path = *patch.basePath();
  auto state = currentState_.wlock();
  updateMetadata(path.begin(), path.end(), *patch.metadata());
  return state->patch(std::move(patch));
}

template <typename Storage, typename SubscribeManager>
std::optional<StorageError>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::patch_impl(
    const fsdb::OperDelta& delta) {
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

template <typename Storage, typename SubscribeManager>
std::optional<StorageError>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::patch_impl(
    const fsdb::TaggedOperState& operState) {
  auto& path = *operState.path()->path();
  auto state = currentState_.wlock();
  auto metadata = operState.state()->metadata() ? *operState.state()->metadata()
                                                : OperMetadata();
  updateMetadata(path.begin(), path.end(), metadata);
  return state->patch(operState);
}

template <typename Storage, typename SubscribeManager>
std::tuple<
    std::shared_ptr<typename NaivePeriodicSubscribableStorage<
        Storage,
        SubscribeManager>::RootNode>,
    std::shared_ptr<typename NaivePeriodicSubscribableStorage<
        Storage,
        SubscribeManager>::RootNode>,
    SubscriptionMetadataServer>
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    publishCurrentState() {
  auto lastState = lastPublishedState_.wlock();
  auto currentState = currentState_.rlock();

  auto oldRoot = lastState->root();
  auto newRoot = currentState->root();
  /*
   * Grab a copy of metadata while holding current state
   * lock. This way we are guaranteed to get metadata
   * corresponding to currentState
   */
  SubscriptionMetadataServer metadataServer = getCurrentMetadataServer();

  if (oldRoot != newRoot) {
    // make sure newRoot is fully published before swapping
    subscriptions_.publishAndAddPaths(newRoot);
  }

  *lastState = Storage(*currentState);
  return std::make_tuple(oldRoot, newRoot, metadataServer);
}

template <typename Storage, typename SubscribeManager>
folly::coro::Task<void> NaivePeriodicSubscribableStorage<
    Storage,
    SubscribeManager>::serveSubscriptions() {
  std::map<std::string, uint64_t> lastServedPublisherRootUpdates;

  while (true) {
    auto start = std::chrono::steady_clock::now();

    if (auto runningLocked = running_.rlock(); !*runningLocked) {
      break;
    }

    auto [oldRoot, newRoot, metadataServer] = publishCurrentState();
    subscriptions_.serveSubscriptions(oldRoot, newRoot, metadataServer);

    exportServeMetrics(start, metadataServer, lastServedPublisherRootUpdates);

    co_await folly::coro::sleep(params_.subscriptionServeInterval_);
  }
}

template <typename Storage, typename SubscribeManager>
typename NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::RootT
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    currentStateExpensive() const {
  return currentState_.rlock()->root()->toThrift();
}

template <typename Storage, typename SubscribeManager>
OperState NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::
    publishedStateEncoded(OperProtocol protocol) {
  auto lastState = Storage(*lastPublishedState_.rlock());
  std::vector<std::string> rootPath;
  return *lastState.get_encoded(rootPath.begin(), rootPath.end(), protocol);
}

template <typename Storage, typename SubscribeManager>
const SubscriptionManagerBase&
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::subMgr() const {
  return subscriptions_;
}

template <typename Storage, typename SubscribeManager>
SubscriptionManagerBase&
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::subMgr() {
  return subscriptions_;
}

template <typename Storage, typename SubscribeManager>
typename Storage::ConcretePath
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::convertPath(
    ConcretePath&& path) const {
  return params_.convertSubsToIDPaths_
      ? PathConverter<RootT>::pathToIdTokens(std::move(path))
      : path;
}

template <typename Storage, typename SubscribeManager>
typename Storage::ExtPath
NaivePeriodicSubscribableStorage<Storage, SubscribeManager>::convertPath(
    const ExtPath& path) const {
  return params_.convertSubsToIDPaths_
      ? PathConverter<RootT>::extPathToIdTokens(path)
      : path;
}

} // namespace facebook::fboss::fsdb
