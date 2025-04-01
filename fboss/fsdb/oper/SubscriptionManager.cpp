// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionManager.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"

namespace facebook::fboss::fsdb {

void SubscriptionManagerBase::registerExtendedSubscription(
    std::shared_ptr<ExtendedSubscription> subscription) {
  if (subscription->type() == PubSubType::PATCH && !useIdPaths_) {
    throw std::runtime_error(
        "Cannot support patch type subscriptions without id paths");
  }
  pendingExtendedSubscriptions_.wlock()->push_back(std::move(subscription));
}

void SubscriptionManagerBase::registerSubscription(
    std::unique_ptr<Subscription> subscription) {
  if (subscription->type() == PubSubType::PATCH && !useIdPaths_) {
    throw std::runtime_error(
        "Cannot support patch type subscriptions without id paths");
  }
  pendingSubscriptions_.wlock()->push_back(std::move(subscription));
}

void SubscriptionManagerBase::pruneCancelledSubscriptions() {
  store_.wlock()->pruneCancelledSubscriptions();
}

void SubscriptionManagerBase::closeNoPublisherActiveSubscriptions(
    const SubscriptionMetadataServer& metadataServer,
    FsdbErrorCode disconnectReason) {
  store_.wlock()->closeNoPublisherActiveSubscriptions(
      metadataServer, disconnectReason);
}

std::vector<OperSubscriberInfo> SubscriptionManagerBase::getSubscriptions()
    const {
  std::vector<OperSubscriberInfo> toRet;
  auto store = store_.rlock();
  toRet.reserve(
      store->subscriptions().size() + store->extendedSubscriptions().size());
  for (auto& [id, subscription] : store->subscriptions()) {
    OperSubscriberInfo info;
    info.subscriberId() = subscription->subscriberId();
    info.type() = subscription->type();
    OperPath p;
    p.raw() = subscription->path();
    info.path() = std::move(p);
    info.subscriptionUid() = subscription->subscriptionUid();
    info.subscriptionQueueWatermark() = subscription->getQueueWatermark();
    toRet.push_back(std::move(info));
  }
  for (auto& [id, subscription] : store->extendedSubscriptions()) {
    OperSubscriberInfo info;
    info.subscriberId() = subscription->subscriberId();
    info.type() = subscription->type();

    auto subscribedPaths = subscription->paths();
    std::vector<ExtendedOperPath> paths(subscribedPaths.size());
    std::transform(
        subscribedPaths.begin(),
        subscribedPaths.end(),
        paths.begin(),
        [](const auto& pair) { return pair.second; });
    info.extendedPaths() = std::move(paths);

    info.subscriptionUid() = subscription->subscriptionUid();
    info.subscriptionQueueWatermark() = subscription->getQueueWatermark();
    info.subscriptionUid() = subscription->subscriptionUid();
    toRet.push_back(std::move(info));
  }
  return toRet;
}

void updateSubscriberStats(
    std::map<FsdbClient, SubscriberStats>& stats,
    const BaseSubscription& subscription) {
  FsdbClient key = *subscriberId2ClientId(subscription.subscriberId()).client();
  auto it = stats.find(key);
  if (it == stats.end()) {
    stats.emplace(key, SubscriberStats());
    it = stats.find(key);
  }
  it->second.numSubscriptions++;
  it->second.subscriptionServeQueueWatermark = std::max(
      it->second.subscriptionServeQueueWatermark,
      subscription.getQueueWatermark());
}

std::map<FsdbClient, SubscriberStats>
SubscriptionManagerBase::getSubscriberStats() const {
  std::map<FsdbClient, SubscriberStats> toRet;
  auto store = store_.rlock();
  for (auto& [id, subscription] : store->subscriptions()) {
    updateSubscriberStats(toRet, *subscription);
  }
  for (auto& [id, subscription] : store->extendedSubscriptions()) {
    updateSubscriberStats(toRet, *subscription);
  }
  return toRet;
}

void SubscriptionManagerBase::registerPendingSubscriptions(
    SubscriptionStore& store) {
  auto pendingSubscriptions = pendingSubscriptions_.wlock();
  auto pendingExtendedSubscriptions = pendingExtendedSubscriptions_.wlock();
  store.registerPendingSubscriptions(
      std::move(*pendingSubscriptions),
      std::move(*pendingExtendedSubscriptions));
  *pendingSubscriptions = PendingSubscriptions();
  *pendingExtendedSubscriptions = PendingExtendedSubscriptions();
}

} // namespace facebook::fboss::fsdb
