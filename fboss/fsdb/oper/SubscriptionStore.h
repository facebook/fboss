// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/Subscription.h"
#include "fboss/fsdb/oper/SubscriptionPathStore.h"

namespace facebook::fboss::fsdb {

// SubscriberStats -- per-client summary counters covering
// active as well as disconnected subscriptions, owned by
// SubscriptionStore. Keeping only summary counters here so
// that callers copy these stats while holding lock on store.
struct SubscriberStats {
  uint32_t numSubscriptions{0};
  uint32_t numExtendedSubscriptions{0};
  uint32_t subscriptionServeQueueWatermark{0};
  uint32_t subscriptionChunksCoalesced{0};
  uint64_t enqueuedDataSize{0};
  uint64_t servedDataSize{0};
  uint32_t numSlowSubscriptionDisconnects{0};
};

class SubscriptionStore {
 public:
  SubscriptionStore()
      : initialSyncNeeded_(&pathStoreStats_), lookup_(&pathStoreStats_) {}

  virtual ~SubscriptionStore();

  void pruneCancelledSubscriptions();

  virtual void registerSubscription(std::unique_ptr<Subscription> subscription);

  void registerExtendedSubscription(
      std::shared_ptr<ExtendedSubscription> subscription);

  void registerPendingSubscriptions(
      std::vector<std::unique_ptr<Subscription>>&& subscriptions,
      std::vector<std::shared_ptr<ExtendedSubscription>>&&
          extendedSubscriptions);

  void unregisterSubscription(const std::string& name);

  void unregisterExtendedSubscription(const std::string& name);

  void processAddedPath(
      std::vector<std::string>::const_iterator begin,
      std::vector<std::string>::const_iterator end);

  void closeNoPublisherActiveSubscriptions(
      const SubscriptionMetadataServer& metadataServer,
      FsdbErrorCode disconnectReason);

  void flush(const SubscriptionMetadataServer& metadataServer);

  const auto& subscriptions() const {
    return subscriptions_;
  }
  const auto& extendedSubscriptions() const {
    return extendedSubscriptions_;
  }
  const auto& initialSyncNeeded() const {
    return initialSyncNeeded_;
  }
  const auto& initialSyncNeededExtended() const {
    return initialSyncNeededExtended_;
  }
  const auto& lookup() const {
    return lookup_;
  }
  auto& subscriptions() {
    return subscriptions_;
  }
  auto& extendedSubscriptions() {
    return extendedSubscriptions_;
  }
  auto& initialSyncNeeded() {
    return initialSyncNeeded_;
  }
  auto& initialSyncNeededExtended() {
    return initialSyncNeededExtended_;
  }
  auto& lookup() {
    return lookup_;
  }

  SubscriptionPathStoreTreeStats* getPathStoreStats() {
    return &pathStoreStats_;
  }

  size_t numPathStoresRecursive_Expensive() const {
    return initialSyncNeeded_.numPathStoresRecursive_Expensive() +
        lookup_.numPathStoresRecursive_Expensive();
  }

  auto numPathStores() const {
    return pathStoreStats_.numPathStores;
  }
  auto numPathStoreAllocs() const {
    return pathStoreStats_.numPathStoreAllocs;
  }
  auto numPathStoreFrees() const {
    return pathStoreStats_.numPathStoreFrees;
  }

  std::map<FsdbClient, SubscriberStats> getSubscriberStats() const;

 private:
  std::map<FsdbClient, SubscriberStats> subscriberStats_;

  std::vector<std::string> markExtendedSubscriptionsThatNeedPruning();

  void pruneSimpleSubscriptions();

  void pruneExtendedSubscriptions(const std::vector<std::string>& toDelete);

  void registerSubscription(
      std::string name,
      std::unique_ptr<Subscription> subscription);

  void registerExtendedSubscription(
      std::string name,
      std::shared_ptr<ExtendedSubscription> subscription);

  // stats for tree<SubscriptionPathStore>
  SubscriptionPathStoreTreeStats pathStoreStats_;

  // owned subscriptions, keyed on name they were registered with
  std::unordered_map<std::string, std::unique_ptr<Subscription>> subscriptions_;
  std::unordered_map<std::string, std::shared_ptr<ExtendedSubscription>>
      extendedSubscriptions_;
  SubscriptionPathStore initialSyncNeeded_;
  std::unordered_set<std::shared_ptr<ExtendedSubscription>>
      initialSyncNeededExtended_;

  // lookup for the subscriptions, keyed on path
  SubscriptionPathStore lookup_;
};

} // namespace facebook::fboss::fsdb
