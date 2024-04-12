// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <any>
#include <optional>

#include <boost/core/noncopyable.hpp>

#include "fboss/fsdb/oper/Subscription.h"

namespace facebook::fboss::fsdb {

namespace {

OperState makeEmptyState(OperProtocol proto) {
  OperState empty;
  empty.protocol() = proto;
  return empty;
}

} // namespace

void BaseDeltaSubscription::appendRootDeltaUnit(const OperDeltaUnit& unit) {
  // this is a helper to add a delta unit in to the currentDelta
  // variable. It assumes the delta is relative to the root of the
  // tree and will pop off path tokens up until the subscribe point.
  //
  // NOTE: this is NOT thread-safe. We expect owner of Subscription
  // objects to manage synchronization.
  if (!currDelta_) {
    currDelta_.emplace();
    currDelta_->protocol() = *protocol_;
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
    SubscriberId subscriber,
    typename DeltaSubscription::PathIter begin,
    typename DeltaSubscription::PathIter end,
    OperProtocol protocol,
    std::optional<std::string> publisherRoot) {
  auto [generator, pipe] = folly::coro::AsyncPipe<OperDelta>::create();
  std::vector<std::string> path(begin, end);
  auto subscription = std::make_unique<DeltaSubscription>(
      std::move(subscriber),
      std::move(path),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot));
  return std::make_pair(std::move(generator), std::move(subscription));
}

void DeltaSubscription::flush(
    const SubscriptionMetadataServer& metadataServer) {
  auto delta = moveFromCurrDelta(metadataServer);
  if (delta) {
    pipe_.write(*delta);
  }
}

void DeltaSubscription::serveHeartbeat() {
  pipe_.write(OperDelta());
}

bool DeltaSubscription::isActive() const {
  return !pipe_.isClosed();
}

void DeltaSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  pipe_.write(Utils::createFsdbException(disconnectReason, msg));
}

FullyResolvedExtendedPathSubscription::FullyResolvedExtendedPathSubscription(
    const std::vector<std::string>& path,
    ExtendedPathSubscription& subscription)
    : BasePathSubscription(
          subscription.subscriberId(),
          path,
          subscription.publisherTreeRoot()),
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

std::optional<OperProtocol>
FullyResolvedExtendedPathSubscription::operProtocol() const {
  return subscription_.operProtocol();
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
    target.oldVal->state() = makeEmptyState(*operProtocol());
  }

  target.newVal.emplace();
  target.newVal->path()->path() = path();
  if (source.newVal) {
    target.newVal->state() = std::move(*source.newVal);
  } else {
    target.newVal->state() = makeEmptyState(*operProtocol());
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
          subscription.subscriberId(),
          path,
          *subscription.operProtocol(),
          subscription.publisherTreeRoot()),
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

std::optional<OperProtocol>
FullyResolvedExtendedDeltaSubscription::operProtocol() const {
  return subscription_.operProtocol();
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
    SubscriberId subscriber,
    std::vector<ExtendedOperPath> paths,
    std::optional<std::string> publisherRoot,
    OperProtocol protocol) {
  auto [generator, pipe] = folly::coro::AsyncPipe<gen_type>::create();
  auto subscription = std::make_shared<ExtendedPathSubscription>(
      std::move(subscriber),
      std::move(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot));
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
  pipe_.write(std::move(toServe).value());
}

std::unique_ptr<Subscription> ExtendedPathSubscription::resolve(
    const std::vector<std::string>& path) {
  return std::make_unique<FullyResolvedExtendedPathSubscription>(path, *this);
}

bool ExtendedPathSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedPathSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  pipe_.write(Utils::createFsdbException(disconnectReason, msg));
}

std::pair<
    folly::coro::AsyncGenerator<typename ExtendedDeltaSubscription::gen_type&&>,
    std::shared_ptr<ExtendedDeltaSubscription>>
ExtendedDeltaSubscription::create(
    SubscriberId subscriber,
    std::vector<ExtendedOperPath> paths,
    std::optional<std::string> publisherRoot,
    OperProtocol protocol) {
  auto [generator, pipe] = folly::coro::AsyncPipe<gen_type>::create();
  auto subscription = std::make_shared<ExtendedDeltaSubscription>(
      std::move(subscriber),
      std::move(paths),
      std::move(pipe),
      std::move(protocol),
      std::move(publisherRoot));
  return std::make_pair(std::move(generator), std::move(subscription));
}

std::unique_ptr<Subscription> ExtendedDeltaSubscription::resolve(
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
  pipe_.write(std::move(toServe).value());
}

void ExtendedDeltaSubscription::serveHeartbeat() {}

bool ExtendedDeltaSubscription::isActive() const {
  return !pipe_.isClosed();
}

void ExtendedDeltaSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  pipe_.write(Utils::createFsdbException(disconnectReason, msg));
}

void PatchSubscription::allPublishersGone(
    FsdbErrorCode disconnectReason,
    const std::string& msg) {
  pipe_.write(Utils::createFsdbException(disconnectReason, msg));
}

std::optional<thrift_cow::Patch> PatchSubscription::moveFromCurrPatch(
    const SubscriptionMetadataServer& /* metadataServer */) {
  std::optional<thrift_cow::Patch> patch = std::move(currPatch_);
  currPatch_.reset();
  return patch;
}

void PatchSubscription::serveHeartbeat() {
  // TODO: heartbeat
}

void PatchSubscription::flush(
    const SubscriptionMetadataServer& /*metadataServer*/) {
  // TODO: write into pipe
}

} // namespace facebook::fboss::fsdb
