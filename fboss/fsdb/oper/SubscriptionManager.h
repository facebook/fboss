// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <any>

#include <boost/core/noncopyable.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_types.h"

#include "fboss/fsdb/oper/DeltaValue.h"
#include "fboss/fsdb/oper/Subscription.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/fsdb/oper/SubscriptionPathStore.h"

namespace facebook::fboss::fsdb {

template <typename _Root, typename Impl>
class SubscriptionManager {
 public:
  using Root = _Root;

  void pruneSimpleSubscriptions() {
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

  std::vector<std::string> markExtendedSubscriptionsThatNeedPruning() {
    std::vector<std::string> toDelete;
    for (auto& [name, subscription] : extendedSubscriptions_) {
      if (subscription->markShouldPruneIfInactive()) {
        toDelete.push_back(name);
      }
    }
    return toDelete;
  }

  void pruneExtendedSubscriptions(const std::vector<std::string>& toDelete) {
    for (const auto& name : toDelete) {
      XLOG(DBG1) << "Removing cancelled extended subscription '" << name
                 << "'...";
      unregisterExtendedSubscription(name);
    }
  }

  void pruneCancelledSubscriptions() {
    // subscriptions also contains FullyResolved*Subscritions which need to be
    // cleaned up BEFORE pruning ExtendedSubscriptions
    auto extendedSubsToPrune = markExtendedSubscriptionsThatNeedPruning();
    pruneSimpleSubscriptions();
    pruneExtendedSubscriptions(extendedSubsToPrune);
  }

  void closeNoPublisherActiveSubscriptions(
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

  void registerExtendedSubscription(
      std::shared_ptr<ExtendedSubscription> subscription) {
    auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    registerExtendedSubscription(uuid, std::move(subscription));
  }

  void registerSubscription(std::unique_ptr<Subscription> subscription) {
    // This flavor of registerSubscription automatically chooses a name.
    auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    const auto& path = subscription->path();
    auto pathStr = folly::join("/", path.begin(), path.end());
    auto candidateName = fmt::format("{}-{}", pathStr, uuid);
    registerSubscription(candidateName, std::move(subscription));
  }

  void unregisterSubscription(const std::string& name) {
    if (auto it = subscriptions_.find(name); it != subscriptions_.end()) {
      auto rawPtr = it->second.get();
      initialSyncNeeded_.erase(rawPtr);
      lookup_.remove(rawPtr);
      subscriptions_.erase(it);
    }
  }

  void unregisterExtendedSubscription(const std::string& name) {
    if (auto it = extendedSubscriptions_.find(name);
        it != extendedSubscriptions_.end()) {
      initialSyncNeededExtended_.erase(it->second);
      extendedSubscriptions_.erase(it);
    }
  }

  size_t numSubscriptions() const {
    return subscriptions_.size();
  }

  size_t numPathStores() const {
    return lookup_.numPathStores();
  }

  std::vector<OperSubscriberInfo> getSubscriptions() const {
    std::vector<OperSubscriberInfo> toRet;
    toRet.reserve(subscriptions_.size());
    for (auto& [id, subscription] : subscriptions_) {
      OperSubscriberInfo info;
      info.subscriberId() = subscription->subscriberId();
      info.type() = subscription->type();
      info.path()->raw() = subscription->path();
      toRet.push_back(std::move(info));
    }
    return toRet;
  }

  void flush(const SubscriptionMetadataServer& metadataServer) {
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

  void serveHeartbeat() {
    for (auto& [_, subscription] : subscriptions_) {
      subscription->serveHeartbeat();
    }
    for (auto& [_, subscription] : extendedSubscriptions_) {
      subscription->serveHeartbeat();
    }
  }

  void useIdPaths(bool idPaths) {
    useIdPaths_ = idPaths;
  }

 private:
  void registerSubscription(
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

  void registerExtendedSubscription(
      std::string name,
      std::shared_ptr<ExtendedSubscription> subscription) {
    XLOG(DBG2) << "Registering extended subscription " << name;
    auto rawPtr = subscription.get();
    auto ret = extendedSubscriptions_.emplace(name, subscription);
    if (!ret.second) {
      throw Utils::createFsdbException(
          FsdbErrorCode::ID_ALREADY_EXISTS, name + " already exixts");
    }
    initialSyncNeededExtended_.insert(std::move(subscription));
  }

 protected:
  void processAddedPath(
      std::vector<std::string>::const_iterator begin,
      std::vector<std::string>::const_iterator end) {
    lookup_.processAddedPath(*this, begin, begin, end);
  }

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
};

} // namespace facebook::fboss::fsdb
