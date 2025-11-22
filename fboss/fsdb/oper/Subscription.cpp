// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/Subscription.h"

#include <boost/core/noncopyable.hpp>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Sleep.h>
#include <optional>

DEFINE_bool(
    forceCloseSlowSubscriber,
    true,
    "Force close slow subscriber if subscription serve queue gets full");

namespace facebook::fboss::fsdb {

namespace {

OperState makeEmptyState(OperProtocol proto) {
  OperState empty;
  empty.protocol() = proto;
  return empty;
}

ExtSubPathMap makeSimplePathMap(std::vector<ExtendedOperPath> paths) {
  ExtSubPathMap map;
  for (auto i = 0; i < paths.size(); ++i) {
    map[i] = std::move(paths[i]);
  }
  return map;
}

} // namespace

BaseSubscription::BaseSubscription(
    SubscriptionIdentifier&& subId,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval)
    : subId_(std::move(subId)),
      protocol_(protocol),
      publisherTreeRoot_(std::move(publisherRoot)),
      heartbeatEvb_(heartbeatEvb),
      heartbeatInterval_(heartbeatInterval),
      streamInfo_(std::make_shared<SubscriptionStreamInfo>()) {
  if (heartbeatEvb_) {
    backgroundScope_.add(co_withExecutor(heartbeatEvb_, heartbeatLoop()));
  }
}

BaseSubscription::~BaseSubscription() {
  // subclasses need to call stop since we have virtual method calls queued
  CHECK(backgroundScope_.isScopeCancellationRequested());
}

void BaseSubscription::stop() {
  folly::coro::blockingWait(backgroundScope_.cancelAndJoinAsync());
}

folly::coro::Task<void> BaseSubscription::heartbeatLoop() {
  // make sure to stop loop when we're cancelled
  while (!backgroundScope_.isScopeCancellationRequested()) {
    co_await folly::coro::sleep(heartbeatInterval_);
    auto ret = serveHeartbeat();
    if (ret.has_value()) {
      requestPruneWithReason(ret.value());
      break;
    }
  }
  co_return;
}

void BaseDeltaSubscription::appendRootDeltaUnit(const OperDeltaUnit& unit) {
  // this is a helper to add a delta unit in to the currentDelta
  // variable. It assumes the delta is relative to the root of the
  // tree and will pop off path tokens up until the subscribe point.
  //
  // NOTE: this is NOT thread-safe. We expect owner of Subscription
  // objects to manage synchronization.
  if (!currDelta_) {
    currDelta_.emplace();
    currDelta_->protocol() = operProtocol();
  }

  OperDeltaUnit toStore;
  toStore.oldState().copy_from(unit.oldState());
  toStore.newState().copy_from(unit.newState());
  toStore.path()->raw() = std::vector<std::string>(
      unit.path()->raw()->begin() + path().size(), unit.path()->raw()->end());
  currDelta_->changes()->emplace_back(std::move(toStore));
}

std::optional<OperDelta> BaseDeltaSubscription::moveFromCurrDelta(
    const SubscriptionMetadataServer& metadataServer) {
  if (!currDelta_) {
    return std::nullopt;
  }

  currDelta_->metadata() = getMetadata(metadataServer);

  std::optional<OperDelta> toServe;
  toServe.swap(currDelta_);
  return toServe;
}

std::pair<
    folly::coro::AsyncGenerator<SubscriptionServeQueueElement<OperDelta>&&>,
    std::unique_ptr<DeltaSubscription>>
DeltaSubscription::create(
    SubscriptionIdentifier&& subscriber,
    typename DeltaSubscription::PathIter begin,
    typename DeltaSubscription::PathIter end,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity,
    size_t subscriptionQueueMemoryLimit,
    int32_t subscriptionQueueFullMinSize) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<
      SubscriptionServeQueueElement<OperDelta>>::create(pipeCapacity);
  std::vector<std::string> path(begin, end);
  auto subscription = std::make_unique<DeltaSubscription>(
      std::move(subscriber),
      std::move(path),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval),
      subscriptionQueueMemoryLimit,
      subscriptionQueueFullMinSize);
  return std::make_pair(std::move(generator), std::move(subscription));
}

std::optional<FsdbErrorCode> DeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  updateMetadata(metadataServer);
  std::optional<FsdbErrorCode> ret;
  auto delta = moveFromCurrDelta(metadataServer);
  if (delta) {
    auto size = getUpdateSize(*delta);
    if (subscriptionQueueMemoryLimit_ > 0) {
      auto queuedChunks = pipe_.getOccupiedSpace();
      if (queuedChunks > subscriptionQueueFullMinSize_) {
        auto streamInfo = getSharedStreamInfo();
        size_t queuedSize = streamInfo->enqueuedDataSize.load() -
            streamInfo->servedDataSize.load();
        if ((queuedSize + size) > subscriptionQueueMemoryLimit_) {
          auto disconnectReason = FsdbErrorCode::SUBSCRIPTION_SERVE_QUEUE_FULL;
          std::string msg = fmt::format(
              "Subscription serve queue memory limit reached: "
              " subscriber: {}, sizeInQueue: {}, nextChunkSize: {}, limit: {}",
              subscriberId(),
              queuedSize,
              size,
              subscriptionQueueMemoryLimit_);
          XLOG(ERR) << msg;
          tryWrite(
              pipe_, Utils::createFsdbException(disconnectReason, msg), 0, "");
          return disconnectReason;
        }
      }
    }
    ret = tryWrite(
        pipe_,
        SubscriptionServeQueueElement<OperDelta>(std::move(*delta), size),
        size,
        "delta.flush");
  }
  return ret;
}

std::optional<FsdbErrorCode> DeltaSubscription::serveHeartbeat() {
  auto delta = OperDelta();
  auto md = getLastMetadata();
  if (md.has_value()) {
    delta.metadata() = md.value();
  }
  return tryWrite(
      pipe_,
      SubscriptionServeQueueElement<OperDelta>(std::move(delta)),
      0 /* updateSize */,
      "delta.hb");
}

bool DeltaSubscription::isActive() const {
  return !pipe_.isClosed();
}

void DeltaSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  tryWrite(
      pipe_,
      Utils::createFsdbException(disconnectReason, msg),
      0 /* updateSize */,
      "delta.pubsGone");
}

FullyResolvedExtendedPathSubscription::FullyResolvedExtendedPathSubscription(
    const std::vector<std::string>& path,
    ExtendedPathSubscription& subscription)
    : BasePathSubscription(
          subscription.subscriptionId(),
          path,
          subscription.operProtocol(),
          subscription.publisherTreeRoot(),
          subscription.heartbeatEvb(),
          subscription.heartbeatInterval()),
      subscription_(subscription) {}

bool FullyResolvedExtendedPathSubscription::isActive() const {
  return subscription_.isActive();
}

bool FullyResolvedExtendedPathSubscription::shouldPrune() const {
  return subscription_.shouldPrune();
}

PubSubType FullyResolvedExtendedPathSubscription::type() const {
  return subscription_.type();
}

std::optional<FsdbErrorCode> FullyResolvedExtendedPathSubscription::offer(
    DeltaValue<OperState> source) {
  // Convert to DeltaValue<TaggedOperState>. Could do this in the
  // SubscriberImpl, but this should do.

  DeltaValue<TaggedOperState> target;
  target.oldVal.emplace();
  target.oldVal->path()->path() = path();
  if (source.oldVal) {
    target.oldVal->state() = std::move(*source.oldVal);
  } else {
    target.oldVal->state() = makeEmptyState(operProtocol());
  }

  target.newVal.emplace();
  target.newVal->path()->path() = path();
  if (source.newVal) {
    target.newVal->state() = std::move(*source.newVal);
  } else {
    target.newVal->state() = makeEmptyState(operProtocol());
  }

  subscription_.buffer(std::move(target));

  return std::nullopt;
}

void FullyResolvedExtendedPathSubscription::allPublishersGone(
    FsdbErrorCode /* disconnectReason */,
    const std::string& /* msg */) {
  // No-op as we expect the extended subscription to send the hangup message
}

std::optional<FsdbErrorCode> FullyResolvedExtendedPathSubscription::flush(
    const SubscriptionMetadataServer& /*metadataServer*/) {
  // no-op as propagation is done in offer()
  return std::nullopt;
}

std::optional<FsdbErrorCode>
FullyResolvedExtendedPathSubscription::serveHeartbeat() {
  return subscription_.serveHeartbeat();
}

FullyResolvedExtendedDeltaSubscription::FullyResolvedExtendedDeltaSubscription(
    const std::vector<std::string>& path,
    ExtendedDeltaSubscription& subscription)
    : BaseDeltaSubscription(
          subscription.subscriptionId(),
          path,
          subscription.operProtocol(),
          subscription.publisherTreeRoot(),
          subscription.heartbeatEvb(),
          subscription.heartbeatInterval()),
      subscription_(subscription) {}

std::optional<FsdbErrorCode> FullyResolvedExtendedDeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  auto delta = moveFromCurrDelta(metadataServer);
  if (delta) {
    TaggedOperDelta target;
    target.path()->path() = path();
    target.delta() = std::move(*delta);

    subscription_.buffer(std::move(target));
  }
  return std::nullopt;
}

std::optional<FsdbErrorCode>
FullyResolvedExtendedDeltaSubscription::serveHeartbeat() {
  return subscription_.serveHeartbeat();
}

bool FullyResolvedExtendedDeltaSubscription::isActive() const {
  return subscription_.isActive();
}

bool FullyResolvedExtendedDeltaSubscription::shouldPrune() const {
  return subscription_.shouldPrune();
}

PubSubType FullyResolvedExtendedDeltaSubscription::type() const {
  return subscription_.type();
}

void FullyResolvedExtendedDeltaSubscription::allPublishersGone(
    FsdbErrorCode /* disconnectReason */,
    const std::string& /* msg */) {
  // No-op as we expect the extended subscription to send the hangup message
}

std::pair<
    folly::coro::AsyncGenerator<typename ExtendedPathSubscription::gen_type&&>,
    std::shared_ptr<ExtendedPathSubscription>>
ExtendedPathSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::vector<ExtendedOperPath> paths,
    std::optional<std::string> publisherRoot,
    OperProtocol protocol,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity) {
  auto [generator, pipe] =
      folly::coro::BoundedAsyncPipe<gen_type>::create(pipeCapacity);
  auto subscription = std::make_shared<ExtendedPathSubscription>(
      std::move(subscriber),
      makeSimplePathMap(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval));
  return std::make_pair(std::move(generator), std::move(subscription));
}

void ExtendedPathSubscription::buffer(
    DeltaValue<ExtendedPathSubscription::value_type>&& newVal) {
  if (!buffered_) {
    buffered_.emplace();
  }
  buffered_->emplace_back(newVal);
}

std::optional<FsdbErrorCode> ExtendedPathSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  updateMetadata(metadataServer);

  if (buffered_) {
    nextOffered_ = std::exchange(buffered_, std::nullopt);
  }

  if (!nextOffered_.has_value()) {
    return std::nullopt;
  }

  return tryCoalescingWrite(pipe_, nextOffered_, "ExtPath.flush");
}

std::unique_ptr<Subscription> ExtendedPathSubscription::resolve(
    const SubscriptionKey& /* key */,
    const std::vector<std::string>& path) {
  return std::make_unique<FullyResolvedExtendedPathSubscription>(path, *this);
}

bool ExtendedPathSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedPathSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  tryWrite(
      pipe_,
      Utils::createFsdbException(disconnectReason, msg),
      0 /* updateSize */,
      "ExtPath.pubsGone");
}

std::pair<
    folly::coro::AsyncGenerator<SubscriptionServeQueueElement<
        typename ExtendedDeltaSubscription::gen_type>&&>,
    std::shared_ptr<ExtendedDeltaSubscription>>
ExtendedDeltaSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::vector<ExtendedOperPath> paths,
    std::optional<std::string> publisherRoot,
    OperProtocol protocol,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity,
    size_t subscriptionQueueMemoryLimit,
    int32_t subscriptionQueueFullMinSize) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<
      SubscriptionServeQueueElement<gen_type>>::create(pipeCapacity);
  auto subscription = std::make_shared<ExtendedDeltaSubscription>(
      std::move(subscriber),
      makeSimplePathMap(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval),
      subscriptionQueueMemoryLimit,
      subscriptionQueueFullMinSize);
  return std::make_pair(std::move(generator), std::move(subscription));
}

std::unique_ptr<Subscription> ExtendedDeltaSubscription::resolve(
    const SubscriptionKey& /* key */,
    const std::vector<std::string>& path) {
  return std::make_unique<FullyResolvedExtendedDeltaSubscription>(path, *this);
}

void ExtendedDeltaSubscription::buffer(TaggedOperDelta&& newVal) {
  if (!buffered_) {
    buffered_.emplace();
  }

  buffered_->emplace_back(newVal);
}

std::optional<FsdbErrorCode> ExtendedDeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  updateMetadata(metadataServer);

  if (!buffered_) {
    return std::nullopt;
  }

  std::optional<gen_type> toServe;
  toServe.swap(buffered_);
  size_t size = getUpdateSize(toServe.value());
  if (subscriptionQueueMemoryLimit_ > 0) {
    auto queuedChunks = pipe_.getOccupiedSpace();
    if (queuedChunks > subscriptionQueueFullMinSize_) {
      auto streamInfo = getSharedStreamInfo();
      size_t queuedSize = streamInfo->enqueuedDataSize.load() -
          streamInfo->servedDataSize.load();
      if ((queuedSize + size) > subscriptionQueueMemoryLimit_) {
        auto disconnectReason = FsdbErrorCode::SUBSCRIPTION_SERVE_QUEUE_FULL;
        std::string msg = fmt::format(
            "Subscription serve queue memory limit reached: "
            " subscriber: {}, sizeInQueue: {}, nextChunkSize: {}, limit: {}",
            subscriberId(),
            queuedSize,
            size,
            subscriptionQueueMemoryLimit_);
        XLOG(ERR) << msg;
        tryWrite(
            pipe_, Utils::createFsdbException(disconnectReason, msg), 0, "");
        return disconnectReason;
      }
    }
  }
  return tryWrite(
      pipe_,
      SubscriptionServeQueueElement<gen_type>(std::move(toServe).value(), size),
      size,
      "ExtDelta.flush");
}

std::optional<FsdbErrorCode> ExtendedDeltaSubscription::serveHeartbeat() {
  return std::nullopt;
}

bool ExtendedDeltaSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedDeltaSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  tryWrite(
      pipe_,
      Utils::createFsdbException(disconnectReason, msg),
      0 /* updateSize */,
      "ExtDelta.pubsGone");
}

PatchSubscription::PatchSubscription(
    const SubscriptionKey& key,
    std::vector<std::string> path,
    ExtendedPatchSubscription& subscription)
    : Subscription(
          subscription.subscriptionId(),
          std::move(path),
          subscription.operProtocol(),
          subscription.publisherTreeRoot(),
          subscription.heartbeatEvb(),
          subscription.heartbeatInterval()),
      key_(key),
      subscription_(subscription) {}

void PatchSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  // No-op as we expect the extended subscription to send the hangup message
}

std::optional<FsdbErrorCode> PatchSubscription::sendEmptyInitialChunk() {
  return subscription_.serveHeartbeat();
}

std::optional<FsdbErrorCode> PatchSubscription::serveHeartbeat() {
  // No-op as we expect the extended subscription to send the heartbeat
  return std::nullopt;
}

std::optional<FsdbErrorCode> PatchSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  // No-op as propagation is done in offer()
  return std::nullopt;
}

std::optional<FsdbErrorCode> PatchSubscription::offer(
    thrift_cow::PatchNode node) {
  Patch patch;
  patch.basePath() = path();
  patch.patch() = std::move(node);
  subscription_.buffer(key_, std::move(patch));
  return std::nullopt;
}

bool PatchSubscription::isActive() const {
  return subscription_.isActive();
}

std::pair<
    folly::coro::AsyncGenerator<
        SubscriptionServeQueueElement<ExtendedPatchSubscription::gen_type>&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::vector<std::string> path,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity,
    size_t subscriptionQueueMemoryLimit,
    int32_t subscriptionQueueFullMinSize) {
  RawOperPath p;
  p.path() = std::move(path);
  return create(
      std::move(subscriber),
      std::map<SubscriptionKey, RawOperPath>{{0, std::move(p)}},
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval),
      pipeCapacity,
      subscriptionQueueMemoryLimit,
      subscriptionQueueFullMinSize);
}

std::pair<
    folly::coro::AsyncGenerator<
        SubscriptionServeQueueElement<ExtendedPatchSubscription::gen_type>&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::map<SubscriptionKey, RawOperPath> paths,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity,
    size_t subscriptionQueueSizeLimit,
    int32_t subscriptionQueueFullMinSize) {
  std::map<SubscriptionKey, ExtendedOperPath> extendedPaths;
  for (auto& [key, path] : paths) {
    std::vector<OperPathElem> extendedPath;
    extendedPath.reserve(path.path()->size());
    for (auto& tok : *path.path()) {
      extendedPath.emplace_back().set_raw(std::move(tok));
    }
    extendedPaths[key].path() = std::move(extendedPath);
  }
  return create(
      std::move(subscriber),
      std::move(extendedPaths),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval),
      pipeCapacity,
      subscriptionQueueSizeLimit,
      subscriptionQueueFullMinSize);
}

std::pair<
    folly::coro::AsyncGenerator<
        SubscriptionServeQueueElement<ExtendedPatchSubscription::gen_type>&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::map<SubscriptionKey, ExtendedOperPath> paths,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval,
    int32_t pipeCapacity,
    size_t subscriptionQueueSizeLimit,
    int32_t subscriptionQueueFullMinSize) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<
      SubscriptionServeQueueElement<gen_type>>::create(pipeCapacity);
  auto subscription = std::make_unique<ExtendedPatchSubscription>(
      std::move(subscriber),
      std::move(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval),
      subscriptionQueueSizeLimit,
      subscriptionQueueFullMinSize);
  return std::make_pair(std::move(generator), std::move(subscription));
}

std::unique_ptr<Subscription> ExtendedPatchSubscription::resolve(
    const SubscriptionKey& key,
    const std::vector<std::string>& path) {
  return std::make_unique<PatchSubscription>(key, path, *this);
}

void ExtendedPatchSubscription::buffer(
    const SubscriptionKey& key,
    Patch&& newVal) {
  buffered_[key].emplace_back(std::move(newVal));
}

std::optional<SubscriberChunk> ExtendedPatchSubscription::moveCurChunk(
    const SubscriptionMetadataServer& metadataServer) {
  if (buffered_.empty()) {
    return std::nullopt;
  }
  for (auto& [_, patchGroups] : buffered_) {
    for (auto& patch : patchGroups) {
      patch.metadata() = getMetadata(metadataServer);
      patch.protocol() = operProtocol();
    }
  }
  SubscriberChunk chunk;
  chunk.patchGroups() = std::move(buffered_);
  buffered_ = {};
  return chunk;
}

std::optional<FsdbErrorCode> ExtendedPatchSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  updateMetadata(metadataServer);
  if (auto chunk = moveCurChunk(metadataServer)) {
    size_t size = getUpdateSize(*chunk);
    if (subscriptionQueueMemoryLimit_ > 0) {
      auto queuedChunks = pipe_.getOccupiedSpace();
      if (queuedChunks > subscriptionQueueFullMinSize_) {
        auto streamInfo = getSharedStreamInfo();
        size_t queuedSize = streamInfo->enqueuedDataSize.load() -
            streamInfo->servedDataSize.load();
        if ((queuedSize + size) > subscriptionQueueMemoryLimit_) {
          auto disconnectReason = FsdbErrorCode::SUBSCRIPTION_SERVE_QUEUE_FULL;
          std::string msg = fmt::format(
              "Subscription serve queue memory limit reached: "
              " subscriber: {}, sizeInQueue: {}, nextChunkSize: {}, limit: {}",
              subscriberId(),
              queuedSize,
              size,
              subscriptionQueueMemoryLimit_);
          XLOG(ERR) << msg;
          tryWrite(
              pipe_, Utils::createFsdbException(disconnectReason, msg), 0, "");
          return disconnectReason;
        }
      }
    }
    SubscriberMessage msg;
    msg.set_chunk(std::move(*chunk));
    return tryWrite(
        pipe_,
        SubscriptionServeQueueElement<gen_type>(std::move(msg), size),
        size,
        "ExtPatch.flush");
  }
  return std::nullopt;
}

std::optional<FsdbErrorCode> ExtendedPatchSubscription::serveHeartbeat() {
  SubscriberMessage msg;
  msg.set_heartbeat();
  auto md = getLastMetadata();
  if (md.has_value()) {
    msg.heartbeat()->metadata() = md.value();
  }
  return tryWrite(
      pipe_,
      SubscriptionServeQueueElement<gen_type>(std::move(msg)),
      0 /* updateSize */,
      "ExtPatch.hb");
}

bool ExtendedPatchSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedPatchSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  tryWrite(
      pipe_,
      Utils::createFsdbException(disconnectReason, msg),
      0 /* updateSize */,
      "ExtPatch.pubsGone");
}

} // namespace facebook::fboss::fsdb
