// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionManager.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void SubscriptionManagerBase::registerExtendedSubscription(
    std::shared_ptr<ExtendedSubscription> subscription) {
  if (subscription->type() == PubSubType::PATCH && !useIdPaths_) {
    throw std::runtime_error(
        "Cannot support patch type subscriptions without id paths");
  }
  store_.registerExtendedSubscription(std::move(subscription));
}

void SubscriptionManagerBase::registerSubscription(
    std::unique_ptr<Subscription> subscription) {
  if (subscription->type() == PubSubType::PATCH && !useIdPaths_) {
    throw std::runtime_error(
        "Cannot support patch type subscriptions without id paths");
  }
  store_.registerSubscription(std::move(subscription));
}

void SubscriptionManagerBase::pruneCancelledSubscriptions() {
  store_.pruneCancelledSubscriptions();
}

std::vector<OperSubscriberInfo> SubscriptionManagerBase::getSubscriptions()
    const {
  std::vector<OperSubscriberInfo> toRet;
  toRet.reserve(store_.subscriptions().size());
  for (auto& [id, subscription] : store_.subscriptions()) {
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

void SubscriptionManagerBase::closeNoPublisherActiveSubscriptions(
    const SubscriptionMetadataServer& metadataServer,
    FsdbErrorCode disconnectReason) {
  store_.closeNoPublisherActiveSubscriptions(metadataServer, disconnectReason);
}

void SubscriptionManagerBase::serveHeartbeat() {
  store_.serveHeartbeat();
}

} // namespace facebook::fboss::fsdb
