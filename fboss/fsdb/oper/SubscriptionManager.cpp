// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionManager.h"
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void SubscriptionManagerBase::pruneSimpleSubscriptions() {
  std::vector<std::string> toDelete;
  for (auto& [name, subscription] : subscriptions_) {
    if (subscription->shouldPrune()) {
      toDelete.push_back(name);
    }
  }

  for (const auto& name : toDelete) {
    XLOG(DBG1) << "Removing cancelled subscription '" << name << "'...";
    unregisterSubscription(name);
  }
}

std::vector<std::string>
SubscriptionManagerBase::markExtendedSubscriptionsThatNeedPruning() {
  std::vector<std::string> toDelete;
  for (auto& [name, subscription] : extendedSubscriptions_) {
    if (subscription->markShouldPruneIfInactive()) {
      toDelete.push_back(name);
    }
  }
  return toDelete;
}

void SubscriptionManagerBase::pruneExtendedSubscriptions(
    const std::vector<std::string>& toDelete) {
  for (const auto& name : toDelete) {
    XLOG(DBG1) << "Removing cancelled extended subscription '" << name
               << "'...";
    unregisterExtendedSubscription(name);
  }
}

void SubscriptionManagerBase::pruneCancelledSubscriptions() {
  // subscriptions also contains FullyResolved*Subscritions which need to be
  // cleaned up BEFORE pruning ExtendedSubscriptions
  auto extendedSubsToPrune = markExtendedSubscriptionsThatNeedPruning();
  pruneSimpleSubscriptions();
  pruneExtendedSubscriptions(extendedSubsToPrune);
}

void SubscriptionManagerBase::closeNoPublisherActiveSubscriptions(
    const SubscriptionMetadataServer& metadataServer,
    FsdbErrorCode disconnectReason) {
  XLOG(DBG2) << " closeSubscriptions: "
             << apache::thrift::util::enumNameSafe(disconnectReason);
  const std::string msg =
      (disconnectReason == FsdbErrorCode::PUBLISHER_GR_DISCONNECT)
      ? "publisher dropped for GR"
      : "All publishers dropped";
  for (auto& [name, subscription] : subscriptions_) {
    if (!metadataServer.getMetadata(subscription->publisherTreeRoot())) {
      subscription->allPublishersGone(disconnectReason, msg);
    }
  }
  for (auto& [name, subscription] : extendedSubscriptions_) {
    if (!metadataServer.getMetadata(subscription->publisherTreeRoot())) {
      subscription->allPublishersGone(disconnectReason, msg);
    }
  }
}

void SubscriptionManagerBase::registerExtendedSubscription(
    std::shared_ptr<ExtendedSubscription> subscription) {
  auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
  registerExtendedSubscription(std::move(uuid), std::move(subscription));
}

void SubscriptionManagerBase::registerSubscription(
    std::unique_ptr<Subscription> subscription) {
  // This flavor of registerSubscription automatically chooses a name.
  auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
  const auto& path = subscription->path();
  auto pathStr = folly::join("/", path.begin(), path.end());
  auto candidateName = fmt::format("{}-{}", pathStr, std::move(uuid));
  registerSubscription(std::move(candidateName), std::move(subscription));
}

void SubscriptionManagerBase::unregisterSubscription(const std::string& name) {
  if (auto it = subscriptions_.find(name); it != subscriptions_.end()) {
    auto rawPtr = it->second.get();
    initialSyncNeeded_.erase(rawPtr);
    lookup_.remove(rawPtr);
    subscriptions_.erase(it);
  }
}

void SubscriptionManagerBase::unregisterExtendedSubscription(
    const std::string& name) {
  if (auto it = extendedSubscriptions_.find(name);
      it != extendedSubscriptions_.end()) {
    initialSyncNeededExtended_.erase(it->second);
    extendedSubscriptions_.erase(it);
  }
}

std::vector<OperSubscriberInfo> SubscriptionManagerBase::getSubscriptions()
    const {
  std::vector<OperSubscriberInfo> toRet;
  toRet.reserve(subscriptions_.size());
  for (auto& [id, subscription] : subscriptions_) {
    OperSubscriberInfo info;
    info.subscriberId() = subscription->subscriberId();
    info.type() = subscription->type();
    OperPath p;
    p.raw() = subscription->path();
    info.path() = std::move(p);
    toRet.push_back(std::move(info));
  }
  return toRet;
}

void SubscriptionManagerBase::flush(
    const SubscriptionMetadataServer& metadataServer) {
  // flushes all subscriptions that need to be flushed.
  //
  // NOTE: it is important we flush regular subscriptions BEFORE
  // extended subscriptions, as some of the regular subscriptions
  // will "flush" in to an extended subscription and be aggregated
  // w/ other changes before flushing out to subscriber.
  //
  // TODO: hint which subscriptions need to be flushed to avoid full
  // loop
  for (auto& [_, subscription] : subscriptions_) {
    subscription->flush(metadataServer);
  }
  for (auto& [_, subscription] : extendedSubscriptions_) {
    subscription->flush(metadataServer);
  }
}

void SubscriptionManagerBase::serveHeartbeat() {
  for (auto& [_, subscription] : subscriptions_) {
    subscription->serveHeartbeat();
  }
  for (auto& [_, subscription] : extendedSubscriptions_) {
    subscription->serveHeartbeat();
  }
}

void SubscriptionManagerBase::registerSubscription(
    std::string name,
    std::unique_ptr<Subscription> subscription) {
  XLOG(DBG2) << "Registering subscription " << name;
  auto rawPtr = subscription.get();
  auto ret = subscriptions_.emplace(name, std::move(subscription));
  if (!ret.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS, name + " already exixts");
  }
  initialSyncNeeded_.insert(rawPtr);
}

void SubscriptionManagerBase::registerExtendedSubscription(
    std::string name,
    std::shared_ptr<ExtendedSubscription> subscription) {
  XLOG(DBG2) << "Registering extended subscription " << name;
  DCHECK(subscription);
  auto ret = extendedSubscriptions_.emplace(name, subscription);
  if (!ret.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS, name + " already exixts");
  }
  initialSyncNeededExtended_.insert(std::move(subscription));
}

void SubscriptionManagerBase::processAddedPath(
    std::vector<std::string>::const_iterator begin,
    std::vector<std::string>::const_iterator end) {
  lookup_.processAddedPath(*this, begin, begin, end);
}

} // namespace facebook::fboss::fsdb
