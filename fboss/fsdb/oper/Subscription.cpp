// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/Subscription.h"

#include <boost/core/noncopyable.hpp>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Sleep.h>
#include <optional>

// default queue size for subscription serve updates
// is chosen to be large enough so that in prod we
// should not see any dropped updates. This value will
// be tuned to lower value after monitoring and max-scale
// testing.
// Rationale for choice of this specific large value:
// * minimum enqueue interval is state subscription serve interval (50msec).
// * Thrift streams holds ~100 updates before pipe queue starts building up.
// So with 1024 default queue size, pipe can get full
// only in the exceptional scenario where subscriber does not
// read any update for > 1 min.
DEFINE_int32(
    subscriptionServeQueueSize,
    1024,
    "Max subscription serve updates to queue, default 1024");

DEFINE_bool(
    forceCloseSlowSubscriber,
    false,
    "Force close slow subscriber if subscription serve queue gets full, default false");

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
      heartbeatInterval_(heartbeatInterval) {
  if (heartbeatEvb_) {
    backgroundScope_.add(heartbeatLoop().scheduleOn(heartbeatEvb_));
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
    serveHeartbeat();
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
    folly::coro::AsyncGenerator<OperDelta&&>,
    std::unique_ptr<DeltaSubscription>>
DeltaSubscription::create(
    SubscriptionIdentifier&& subscriber,
    typename DeltaSubscription::PathIter begin,
    typename DeltaSubscription::PathIter end,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<OperDelta>::create(
      FLAGS_subscriptionServeQueueSize);
  std::vector<std::string> path(begin, end);
  auto subscription = std::make_unique<DeltaSubscription>(
      std::move(subscriber),
      std::move(path),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval));
  return std::make_pair(std::move(generator), std::move(subscription));
}

void DeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  auto delta = moveFromCurrDelta(metadataServer);
  if (delta) {
    tryWrite(pipe_, std::move(*delta), "delta.flush");
  }
}

void DeltaSubscription::serveHeartbeat() {
  tryWrite(pipe_, OperDelta(), "delta.hb");
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

void FullyResolvedExtendedPathSubscription::offer(
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
}

void FullyResolvedExtendedPathSubscription::allPublishersGone(
    FsdbErrorCode /* disconnectReason */,
    const std::string& /* msg */) {
  // No-op as we expect the extended subscription to send the hangup message
}

void FullyResolvedExtendedPathSubscription::flush(
    const SubscriptionMetadataServer& /*metadataServer*/) {
  // no-op as propagation is done in offer()
}

void FullyResolvedExtendedPathSubscription::serveHeartbeat() {
  subscription_.serveHeartbeat();
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

void FullyResolvedExtendedDeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  auto delta = moveFromCurrDelta(metadataServer);
  if (!delta) {
    return;
  }

  TaggedOperDelta target;
  target.path()->path() = path();
  target.delta() = std::move(*delta);

  subscription_.buffer(std::move(target));
}

void FullyResolvedExtendedDeltaSubscription::serveHeartbeat() {
  subscription_.serveHeartbeat();
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
    std::chrono::milliseconds heartbeatInterval) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<gen_type>::create(
      FLAGS_subscriptionServeQueueSize);
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

void ExtendedPathSubscription::flush(
    const SubscriptionMetadataServer& /*metadataServer*/) {
  if (!buffered_) {
    return;
  }

  std::optional<gen_type> toServe;
  toServe.swap(buffered_);
  tryWrite(pipe_, std::move(toServe).value(), "ExtPath.flush");
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
      "ExtPath.pubsGone");
}

std::pair<
    folly::coro::AsyncGenerator<typename ExtendedDeltaSubscription::gen_type&&>,
    std::shared_ptr<ExtendedDeltaSubscription>>
ExtendedDeltaSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::vector<ExtendedOperPath> paths,
    std::optional<std::string> publisherRoot,
    OperProtocol protocol,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<gen_type>::create(
      FLAGS_subscriptionServeQueueSize);
  auto subscription = std::make_shared<ExtendedDeltaSubscription>(
      std::move(subscriber),
      makeSimplePathMap(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval));
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

void ExtendedDeltaSubscription::flush(
    const SubscriptionMetadataServer& /*metadataServer*/) {
  if (!buffered_) {
    return;
  }

  std::optional<gen_type> toServe;
  toServe.swap(buffered_);
  tryWrite(pipe_, std::move(toServe).value(), "ExtDelta.flush");
}

void ExtendedDeltaSubscription::serveHeartbeat() {}

bool ExtendedDeltaSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedDeltaSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  tryWrite(
      pipe_,
      Utils::createFsdbException(disconnectReason, msg),
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

void PatchSubscription::sendEmptyInitialChunk() {
  subscription_.serveHeartbeat();
}

void PatchSubscription::serveHeartbeat() {
  // No-op as we expect the extended subscription to send the heartbeat
}

void PatchSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  // No-op as propagation is done in offer()
}

void PatchSubscription::offer(thrift_cow::PatchNode node) {
  Patch patch;
  patch.basePath() = path();
  patch.patch() = std::move(node);
  subscription_.buffer(key_, std::move(patch));
}

bool PatchSubscription::isActive() const {
  return subscription_.isActive();
}

std::pair<
    folly::coro::AsyncGenerator<ExtendedPatchSubscription::gen_type&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::vector<std::string> path,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval) {
  RawOperPath p;
  p.path() = std::move(path);
  return create(
      std::move(subscriber),
      std::map<SubscriptionKey, RawOperPath>{{0, std::move(p)}},
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval));
}

std::pair<
    folly::coro::AsyncGenerator<ExtendedPatchSubscription::gen_type&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::map<SubscriptionKey, RawOperPath> paths,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval) {
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
      std::move(heartbeatInterval));
}

std::pair<
    folly::coro::AsyncGenerator<ExtendedPatchSubscription::gen_type&&>,
    std::unique_ptr<ExtendedPatchSubscription>>
ExtendedPatchSubscription::create(
    SubscriptionIdentifier&& subscriber,
    std::map<SubscriptionKey, ExtendedOperPath> paths,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot,
    folly::EventBase* heartbeatEvb,
    std::chrono::milliseconds heartbeatInterval) {
  auto [generator, pipe] = folly::coro::BoundedAsyncPipe<gen_type>::create(
      FLAGS_subscriptionServeQueueSize);
  auto subscription = std::make_unique<ExtendedPatchSubscription>(
      std::move(subscriber),
      std::move(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot),
      std::move(heartbeatEvb),
      std::move(heartbeatInterval));
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

void ExtendedPatchSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  if (auto chunk = moveCurChunk(metadataServer)) {
    SubscriberMessage msg;
    msg.set_chunk(std::move(*chunk));
    tryWrite(pipe_, std::move(msg), "ExtPatch.flush");
  }
}

void ExtendedPatchSubscription::serveHeartbeat() {
  SubscriberMessage msg;
  msg.set_heartbeat();
  tryWrite(pipe_, std::move(msg), "ExtPatch.hb");
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
      "ExtPatch.pubsGone");
}

} // namespace facebook::fboss::fsdb
