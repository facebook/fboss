// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionStore.h"
#include "fboss/fsdb/common/Utils.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void updateSubscriberStats(
    std::map<FsdbClient, SubscriberStats>& stats,
    const BaseSubscription& subscription,
    const std::function<void(SubscriberStats&, const BaseSubscription&)>&
        updater) {
  FsdbClient key = *subscriberId2ClientId(subscription.subscriberId()).client();
  auto it = stats.find(key);
  if (it == stats.end()) {
    stats.emplace(key, SubscriberStats());
    it = stats.find(key);
  }
  updater(it->second, subscription);
}

SubscriptionStore::~SubscriptionStore() {
  initialSyncNeeded_.clear(&pathStoreStats_);
  initialSyncNeededExtended_.clear();

  lookup_.clear(&pathStoreStats_);
  // fully resolved extended subs have a ref to the extended sub
  // make sure to destroy those before destroy the extended sub
  subscriptions_.clear();
  extendedSubscriptions_.clear();
}

std::string getPublisherDroppedMessage(
    FsdbErrorCode disconnectReason,
    std::string pubRoot) {
  return (disconnectReason == FsdbErrorCode::PUBLISHER_GR_DISCONNECT)
      ? fmt::format("publisher dropped for GR for root: {}", std::move(pubRoot))
      : fmt::format("All publishers dropped for root: {}", std::move(pubRoot));
}

void SubscriptionStore::pruneSimpleSubscriptions() {
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
SubscriptionStore::markExtendedSubscriptionsThatNeedPruning() {
  std::vector<std::string> toDelete;
  for (auto& [name, subscription] : extendedSubscriptions_) {
    if (subscription->markShouldPruneIfInactive()) {
      toDelete.push_back(name);
    }
  }
  return toDelete;
}

void SubscriptionStore::pruneExtendedSubscriptions(
    const std::vector<std::string>& toDelete) {
  for (const auto& name : toDelete) {
    XLOG(DBG1) << "Removing cancelled extended subscription '" << name
               << "'...";
    unregisterExtendedSubscription(name);
  }
}

void SubscriptionStore::pruneCancelledSubscriptions() {
  // subscriptions also contains FullyResolved*Subscritions which need to be
  // cleaned up BEFORE pruning ExtendedSubscriptions
  auto extendedSubsToPrune = markExtendedSubscriptionsThatNeedPruning();
  pruneSimpleSubscriptions();
  pruneExtendedSubscriptions(extendedSubsToPrune);
}

void SubscriptionStore::registerExtendedSubscription(
    std::shared_ptr<ExtendedSubscription> subscription) {
  auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
  registerExtendedSubscription(std::move(uuid), std::move(subscription));
}

void SubscriptionStore::registerSubscription(
    std::unique_ptr<Subscription> subscription) {
  // This flavor of registerSubscription automatically chooses a name.
  auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
  const auto& path = subscription->path();
  auto pathStr = folly::join("/", path.begin(), path.end());
  auto candidateName = fmt::format("{}-{}", pathStr, std::move(uuid));
  registerSubscription(std::move(candidateName), std::move(subscription));
}

void SubscriptionStore::registerPendingSubscriptions(
    std::vector<std::unique_ptr<Subscription>>&& subscriptions,
    std::vector<std::shared_ptr<ExtendedSubscription>>&&
        extendedSubscriptions) {
  for (auto& subscription : subscriptions) {
    registerSubscription(std::move(subscription));
  }
  for (auto& extendedSubscription : extendedSubscriptions) {
    registerExtendedSubscription(std::move(extendedSubscription));
  }
}

void updateSubscriberStatsDisconnectReason(
    SubscriberStats& stats,
    const BaseSubscription& subscription) {
  auto pruneReason = subscription.pruneReason();
  if (pruneReason) {
    if (pruneReason.value() == FsdbErrorCode::SUBSCRIPTION_SERVE_QUEUE_FULL) {
      stats.numSlowSubscriptionDisconnects++;
    }
  }
}

void SubscriptionStore::unregisterSubscription(const std::string& name) {
  XLOG(DBG1) << "Unregistering subscription " << name;
  if (auto it = subscriptions_.find(name); it != subscriptions_.end()) {
    Subscription* rawPtr = it->second.get();
    if (rawPtr->pruneReason()) {
      updateSubscriberStats(
          subscriberStats_, *rawPtr, updateSubscriberStatsDisconnectReason);
    }
    // TODO: trim empty path stores
    initialSyncNeeded_.remove(rawPtr);
    lookup_.remove(rawPtr);
    subscriptions_.erase(it);
  }
}

void SubscriptionStore::unregisterExtendedSubscription(
    const std::string& name) {
  XLOG(DBG1) << "Unregistering extended subscription " << name;
  if (auto it = extendedSubscriptions_.find(name);
      it != extendedSubscriptions_.end()) {
    if (it->second.get()->pruneReason()) {
      updateSubscriberStats(
          subscriberStats_,
          *it->second.get(),
          updateSubscriberStatsDisconnectReason);
    }
    initialSyncNeededExtended_.erase(it->second);
    extendedSubscriptions_.erase(it);
  }
}

void SubscriptionStore::registerSubscription(
    std::string name,
    std::unique_ptr<Subscription> subscription) {
  XLOG(DBG1) << "Registering subscription " << name;
  auto rawPtr = subscription.get();
  auto ret = subscriptions_.emplace(name, std::move(subscription));
  if (!ret.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS, name + " already exixts");
  }
  initialSyncNeeded_.add(rawPtr, &pathStoreStats_);
}

void SubscriptionStore::registerExtendedSubscription(
    std::string name,
    std::shared_ptr<ExtendedSubscription> subscription) {
  XLOG(DBG1) << "Registering extended subscription " << name;
  DCHECK(subscription);
  auto ret = extendedSubscriptions_.emplace(name, subscription);
  if (!ret.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS, name + " already exixts");
  }
  initialSyncNeededExtended_.insert(std::move(subscription));
}

void SubscriptionStore::closeNoPublisherActiveSubscriptions(
    const SubscriptionMetadataServer& metadataServer,
    FsdbErrorCode disconnectReason) {
  XLOG(DBG2) << " closeSubscriptions: "
             << apache::thrift::util::enumNameSafe(disconnectReason);
  for (auto& [name, subscription] : subscriptions_) {
    if (!metadataServer.getMetadata(subscription->publisherTreeRoot())) {
      subscription->allPublishersGone(
          disconnectReason,
          getPublisherDroppedMessage(
              disconnectReason, subscription->publisherTreeRoot()));
    }
  }
  for (auto& [name, subscription] : extendedSubscriptions_) {
    if (!metadataServer.getMetadata(subscription->publisherTreeRoot())) {
      subscription->allPublishersGone(
          disconnectReason,
          getPublisherDroppedMessage(
              disconnectReason, subscription->publisherTreeRoot()));
    }
  }
}

void SubscriptionStore::flush(
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
    auto ret = subscription->flush(metadataServer);
    if (ret.has_value()) {
      subscription->requestPruneWithReason(ret.value());
    }
  }
  for (auto& [_, subscription] : extendedSubscriptions_) {
    auto ret = subscription->flush(metadataServer);
    if (ret.has_value()) {
      subscription->requestPruneWithReason(ret.value());
    }
  }
}

void SubscriptionStore::processAddedPath(
    std::vector<std::string>::const_iterator begin,
    std::vector<std::string>::const_iterator end) {
  lookup_.processAddedPath(*this, begin, begin, end);
}

std::map<FsdbClient, SubscriberStats> SubscriptionStore::getSubscriberStats()
    const {
  std::map<FsdbClient, SubscriberStats> toRet = folly::copy(subscriberStats_);
  auto updater = [](SubscriberStats& stats,
                    const BaseSubscription& subscription) {
    stats.numSubscriptions++;
    stats.subscriptionServeQueueWatermark = std::max(
        stats.subscriptionServeQueueWatermark,
        subscription.getQueueWatermark());
    stats.subscriptionChunksCoalesced = subscription.getChunksCoalesced();
    stats.enqueuedDataSize = subscription.getEnqueuedDataSize();
    stats.servedDataSize = subscription.getServedDataSize();
  };
  for (auto& [id, subscription] : subscriptions_) {
    updateSubscriberStats(toRet, *subscription, updater);
  }
  for (auto& [id, subscription] : extendedSubscriptions_) {
    updateSubscriberStats(toRet, *subscription, updater);
  }
  return toRet;
}

} // namespace facebook::fboss::fsdb
