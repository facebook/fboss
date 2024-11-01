// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_types.h"
#include "fboss/fsdb/oper/Subscription.h"
#include "fboss/fsdb/oper/SubscriptionStore.h"

#include <folly/logging/xlog.h>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

class SubscriptionMetadataServer;

class SubscriptionManagerBase {
 public:
  explicit SubscriptionManagerBase(
      OperProtocol patchOperProtocol = OperProtocol::COMPACT,
      bool requireResponseOnInitialSync = false)
      : patchOperProtocol_(patchOperProtocol),
        requireResponseOnInitialSync_(requireResponseOnInitialSync) {}

  void pruneCancelledSubscriptions();

  void closeNoPublisherActiveSubscriptions(
      const SubscriptionMetadataServer& metadataServer,
      FsdbErrorCode disconnectReason);

  void registerExtendedSubscription(
      std::shared_ptr<ExtendedSubscription> subscription);

  void registerSubscription(std::unique_ptr<Subscription> subscription);

  size_t numSubscriptions() const {
    return store_.rlock()->subscriptions().size();
  }

  // Do not use, except for UTs that cross check numPathStores()
  size_t numPathStoresRecursive_Expensive() const {
    return store_.rlock()->numPathStoresRecursive_Expensive();
  }

  size_t numPathStores() const {
    return store_.rlock()->numPathStores();
  }

  uint64_t numPathStoreAllocs() const {
    return store_.rlock()->numPathStoreAllocs();
  }

  uint64_t numPathStoreFrees() const {
    return store_.rlock()->numPathStoreFrees();
  }

  std::vector<OperSubscriberInfo> getSubscriptions() const;

  void useIdPaths(bool idPaths) {
    useIdPaths_ = idPaths;
  }

  // Serve response (even if empty) on initial sync, even when
  // no data is published for subscribed path
  void setRequireResponseOnInitialSync(bool requireResponse) {
    requireResponseOnInitialSync_ = requireResponse;
  }

  OperProtocol patchOperProtocol() {
    return patchOperProtocol_;
  }

 private:
  void registerSubscription(
      std::string name,
      std::unique_ptr<Subscription> subscription);

  void registerExtendedSubscription(
      std::string name,
      std::shared_ptr<ExtendedSubscription> subscription);

 protected:
  void registerPendingSubscriptions(SubscriptionStore& store);

  folly::Synchronized<SubscriptionStore> store_;

  bool useIdPaths_{false};

  const OperProtocol patchOperProtocol_{OperProtocol::COMPACT};
  bool requireResponseOnInitialSync_{false};

 private:
  using PendingSubscriptions = std::vector<std::unique_ptr<Subscription>>;
  using PendingExtendedSubscriptions =
      std::vector<std::shared_ptr<ExtendedSubscription>>;
  folly::Synchronized<PendingSubscriptions> pendingSubscriptions_;
  folly::Synchronized<PendingExtendedSubscriptions>
      pendingExtendedSubscriptions_;
};

template <typename _Root, typename Impl>
class SubscriptionManager : public SubscriptionManagerBase {
 public:
  using Root = _Root;

  using SubscriptionManagerBase::SubscriptionManagerBase;

  void publishAndAddPaths(std::shared_ptr<Root>& root) {
    auto store = store_.wlock();
    static_cast<Impl*>(this)->publishAndAddPaths(*store, root);
  }

  void serveSubscriptions(
      const std::shared_ptr<Root>& oldRoot,
      const std::shared_ptr<Root>& newRoot,
      const SubscriptionMetadataServer& metadataServer) {
    auto impl = static_cast<Impl*>(this);
    auto store = store_.wlock();

    registerPendingSubscriptions(*store);

    store->pruneCancelledSubscriptions();

    if (oldRoot != newRoot) {
      try {
        impl->serveSubscriptions(*store, oldRoot, newRoot, metadataServer);
      } catch (const std::exception&) {
        XLOG(ERR) << "Exception serving subscriptions...";
      }
      impl->pruneDeletedPaths(*store, oldRoot, newRoot);
    }
    // Serve new subscriptions after serving existing subscriptions.
    // New subscriptions will get a full object dump on first sync.
    // If we serve them before the loop above, we have to be careful
    // to not serve them again in the loop above. So just move to serve
    // after. Post the initial sync, these new subscriptions will be
    // pruned from initialSyncNeeded list and will get served on
    // changes only
    impl->doInitialSync(*store, newRoot, metadataServer);
    // Flush all subscription queues from serve and initial sync steps
    store->flush(metadataServer);
  }

 private:
  // don't let the subclass direct access to the store to simplify locking
  // policy. Instead we'll handle all the locking of the store here
  using SubscriptionManagerBase::store_;
};

} // namespace facebook::fboss::fsdb
