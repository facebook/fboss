// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_types.h"
#include "fboss/fsdb/oper/Subscription.h"
#include "fboss/fsdb/oper/SubscriptionPathStore.h"

#include <folly/logging/xlog.h>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

class SubscriptionMetadataServer;

class SubscriptionManagerBase {
 public:
  virtual ~SubscriptionManagerBase() = default;

  void pruneSimpleSubscriptions();

  std::vector<std::string> markExtendedSubscriptionsThatNeedPruning();

  void pruneExtendedSubscriptions(const std::vector<std::string>& toDelete);

  void pruneCancelledSubscriptions();

  void closeNoPublisherActiveSubscriptions(
      const SubscriptionMetadataServer& metadataServer,
      FsdbErrorCode disconnectReason);

  void registerExtendedSubscription(
      std::shared_ptr<ExtendedSubscription> subscription);

  virtual void registerSubscription(std::unique_ptr<Subscription> subscription);

  void unregisterSubscription(const std::string& name);

  void unregisterExtendedSubscription(const std::string& name);

  size_t numSubscriptions() const {
    return subscriptions_.size();
  }

  size_t numPathStores() const {
    return lookup_.numPathStores();
  }

  std::vector<OperSubscriberInfo> getSubscriptions() const;

  void flush(const SubscriptionMetadataServer& metadataServer);

  void serveHeartbeat();

  void useIdPaths(bool idPaths) {
    useIdPaths_ = idPaths;
  }

  // Serve response (even if empty) on initial sync, even when
  // no data is published for subscribed path
  void setRequireResponseOnInitialSync(bool requireResponse) {
    requireResponseOnInitialSync_ = requireResponse;
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

  // owned subscriptions, keyed on name they were registered with
  std::unordered_map<std::string, std::unique_ptr<Subscription>> subscriptions_;
  std::unordered_map<std::string, std::shared_ptr<ExtendedSubscription>>
      extendedSubscriptions_;
  std::unordered_set<Subscription*> initialSyncNeeded_;
  std::unordered_set<std::shared_ptr<ExtendedSubscription>>
      initialSyncNeededExtended_;

  // lookup for the subscriptions, keyed on path
  SubscriptionPathStore lookup_;

  bool useIdPaths_{false};

  bool requireResponseOnInitialSync_{false};
};

template <typename _Root, typename Impl>
class SubscriptionManager : public SubscriptionManagerBase {
 public:
  using Root = _Root;

  void initialSyncForNewSubscriptions(
      const Root& newData,
      const SubscriptionMetadataServer& metadataServer) {
    static_cast<Impl*>(this)->doInitialSync(newData, metadataServer);
  }

  // TODO: hinting at changed paths for improved efficiency
  void serveSubscriptions(
      const Root& oldData,
      const Root& newData,
      const SubscriptionMetadataServer& metadataServer) {
    try {
      static_cast<Impl*>(this)->serveSubscriptions(
          oldData, newData, metadataServer);
    } catch (const std::exception&) {
      XLOG(ERR) << "Exception serving subscriptions...";
    }
  }
};

} // namespace facebook::fboss::fsdb
