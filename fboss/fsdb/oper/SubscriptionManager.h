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
    return store_.subscriptions().size();
  }

  size_t numPathStores() const {
    return store_.lookup().numPathStores();
  }

  std::vector<OperSubscriberInfo> getSubscriptions() const;

  void serveHeartbeat();

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
  void processAddedPath(
      std::vector<std::string>::const_iterator begin,
      std::vector<std::string>::const_iterator end);

  SubscriptionStore store_;

  // lookup for the subscriptions, keyed on path
  SubscriptionPathStore lookup_;

  bool useIdPaths_{false};

  const OperProtocol patchOperProtocol_{OperProtocol::COMPACT};
  bool requireResponseOnInitialSync_{false};
};

template <typename _Root, typename Impl>
class SubscriptionManager : public SubscriptionManagerBase {
 public:
  using Root = _Root;

  using SubscriptionManagerBase::SubscriptionManagerBase;

  void publishAndAddPaths(std::shared_ptr<Root>& root) {
    static_cast<Impl*>(this)->publishAndAddPaths(store_, root);
  }

  // TODO: hinting at changed paths for improved efficiency
  void serveSubscriptions(
      const std::shared_ptr<Root>& oldData,
      const std::shared_ptr<Root>& newData,
      const SubscriptionMetadataServer& metadataServer) {
    try {
      static_cast<Impl*>(this)->serveSubscriptions(
          store_, oldData, newData, metadataServer);
    } catch (const std::exception&) {
      XLOG(ERR) << "Exception serving subscriptions...";
    }
  }

  void pruneDeletedPaths(
      const std::shared_ptr<Root>& oldRoot,
      const std::shared_ptr<Root>& newRoot) {
    static_cast<Impl*>(this)->pruneDeletedPaths(
        &store_.lookup(), oldRoot, newRoot);
  }

  void initialSyncForNewSubscriptions(
      const std::shared_ptr<Root>& newData,
      const SubscriptionMetadataServer& metadataServer) {
    static_cast<Impl*>(this)->doInitialSync(store_, newData, metadataServer);
  }

 private:
  // don't let the subclass direct access to the store to simplify locking
  // policy. Instead we'll handle all the locking of the store here
  using SubscriptionManagerBase::store_;
};

} // namespace facebook::fboss::fsdb
