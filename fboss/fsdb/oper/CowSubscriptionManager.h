/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/fsdb/common/Utils.h>
#include <fboss/fsdb/oper/CowDeletePathTraverseHelper.h>
#include <fboss/fsdb/oper/CowPublishAndAddTraverseHelper.h>
#include <fboss/fsdb/oper/CowSubscriptionTraverseHelper.h>
#include <fboss/fsdb/oper/SubscriptionManager.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/ExtendedPathVisitor.h>
#include <fboss/thrift_cow/visitors/PathVisitor.h>
#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <folly/Traits.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_fatal_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

namespace {

template <typename T>
struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

} // namespace

namespace csm_detail {
template <typename NodeT>
class OperDeltaUnitCache {
 public:
  OperDeltaUnitCache(
      const std::vector<std::string>& path,
      const NodeT& oldNode,
      const NodeT& newNode)
      : path_(path), oldNode_(oldNode), newNode_(newNode) {}

  const OperDeltaUnit& binaryUnit() {
    return getOrBuild(binaryUnit_, fsdb::OperProtocol::BINARY);
  }

  const OperDeltaUnit& compactUnit() {
    return getOrBuild(compactUnit_, fsdb::OperProtocol::COMPACT);
  }

  const OperDeltaUnit& jsonUnit() {
    return getOrBuild(jsonUnit_, fsdb::OperProtocol::SIMPLE_JSON);
  }

  const OperDeltaUnit& getEncoded(fsdb::OperProtocol protocol) {
    switch (protocol) {
      case fsdb::OperProtocol::BINARY:
        return binaryUnit();
      case fsdb::OperProtocol::COMPACT:
        return compactUnit();
      case fsdb::OperProtocol::SIMPLE_JSON:
        return jsonUnit();
    }
  }

 private:
  const OperDeltaUnit& getOrBuild(
      std::optional<OperDeltaUnit>& unit,
      fsdb::OperProtocol protocol) {
    if (!unit.has_value()) {
      unit = buildOperDeltaUnit(path_, oldNode_, newNode_, protocol);
    }
    return *unit;
  }

  const std::vector<std::string>& path_;
  const NodeT& oldNode_;
  const NodeT& newNode_;
  std::optional<OperDeltaUnit> binaryUnit_, compactUnit_, jsonUnit_;
};
} // namespace csm_detail

template <typename _Root>
class CowSubscriptionManager
    : public SubscriptionManager<_Root, CowSubscriptionManager<_Root>> {
 public:
  using Root = _Root;

 private:
  template <typename NodeT>
  void servePathEncoded(
      BasePathSubscription* subscription,
      NodeT&& oldNode,
      NodeT&& newNode,
      OperProtocol protocol,
      const Root& newStorage,
      const SubscriptionMetadataServer& metadataServer) {
    std::optional<OperState> oldState, newState;
    if (oldNode) {
      oldState.emplace();
      oldState->contents() = oldNode->encode(protocol);
      oldState->protocol() = protocol;
      // No metadata for oldState, we don't maintain metadata
      // history
    }
    if (newNode) {
      newState.emplace();
      newState->contents() = newNode->encode(protocol);
      newState->protocol() = protocol;
      newState->metadata() = subscription->getMetadata(metadataServer);
    }
    auto value = DeltaValue<OperState>(oldState, newState);
    subscription->offer(std::any(std::move(value)));
  }

  void doInitialSyncSimple(
      const Root& newRoot,
      const SubscriptionMetadataServer& metadataServer) {
    auto it = this->initialSyncNeeded_.begin();
    while (it != this->initialSyncNeeded_.end()) {
      auto& subscription = *it;

      if (!metadataServer.ready(subscription->publisherTreeRoot())) {
        ++it;
        continue;
      }

      const auto& path = subscription->path();

      thrift_cow::GetEncodedPathVisitorOperator op(
          *subscription->operProtocol());
      const auto& root = *newRoot.root();
      thrift_cow::RootPathVisitor::visit(
          root, path.begin(), path.end(), thrift_cow::PathVisitMode::LEAF, op);
      if (op.val) {
        if (subscription->type() == PubSubType::PATH) {
          auto pathSubscription =
              static_cast<BasePathSubscription*>(subscription);
          if (auto proto = subscription->operProtocol(); proto) {
            std::optional<OperState> state;
            state.emplace();
            state->contents() = std::move(*op.val);
            state->protocol() = *proto;
            state->metadata() = subscription->getMetadata(metadataServer);
            auto value = DeltaValue<OperState>(std::nullopt, std::move(state));
            pathSubscription->offer(std::any(std::move(value)));
          }
        } else {
          // Delta subscription
          CHECK(subscription->operProtocol());
          OperDeltaUnit deltaUnit;
          deltaUnit.path()->raw() = path;
          deltaUnit.newState() = std::move(*op.val);
          auto deltaSubscription =
              static_cast<BaseDeltaSubscription*>(subscription);
          deltaSubscription->appendRootDeltaUnit(std::move(deltaUnit));
          deltaSubscription->flush(metadataServer);
        }
      } else if (this->requireResponseOnInitialSync_) {
        // on initialSync, offer even an empty value
        subscription->serveHeartbeat();
      }

      this->lookup_.add(subscription);
      it = this->initialSyncNeeded_.erase(it);
    }
  }

  void doInitialSyncExtended(
      const Root& newRoot,
      const SubscriptionMetadataServer& metadataServer) {
    const auto& root = *newRoot.root();
    auto process = [&](const auto& path, auto& node) {
      this->processAddedPath(path.begin(), path.end());
    };

    auto it = this->initialSyncNeededExtended_.begin();
    while (it != this->initialSyncNeededExtended_.end()) {
      auto& subscription = *it;

      if (!metadataServer.ready(subscription->publisherTreeRoot())) {
        ++it;
        continue;
      }

      const auto& paths = subscription->paths();
      for (int pathNum = 0; pathNum < paths.size(); ++pathNum) {
        // seed beginnings of the path in to lookup tree
        std::vector<std::string> emptyPathSoFar;
        this->lookup_.incrementallyResolve(
            *this, subscription, pathNum, emptyPathSoFar);

        const auto& path = paths.at(pathNum);
        thrift_cow::ExtPathVisitorOptions options(this->useIdPaths_);
        thrift_cow::RootExtendedPathVisitor::visit(
            root, path.path()->begin(), path.path()->end(), options, process);
      }
      it = this->initialSyncNeededExtended_.erase(it);
    }
  }

 public:
  void publishAndAddPaths(Root& storage) {
    // this helper recurses through all unpublished paths and ensures
    // that we tell SubscriptionPathStore of any new paths.
    auto processPath = [&](const std::vector<std::string>& /*path*/,
                           auto&& node) mutable {
      if constexpr (is_shared_ptr_v<folly::remove_cvref_t<decltype(node)>>) {
        // We only want to publish the node itself, not recurse to
        // children.  This invokes the base version of publish to
        // avoid recursing automatically.
        node->NodeBase::publish();
      }
    };

    auto root = storage.root();
    CowPublishAndAddTraverseHelper traverser(&this->lookup_, this);
    thrift_cow::RootRecurseVisitor::visit(
        traverser,
        root,
        thrift_cow::RecurseVisitOptions(
            thrift_cow::RecurseVisitMode::UNPUBLISHED,
            thrift_cow::RecurseVisitOrder::CHILDREN_FIRST,
            this->useIdPaths_),
        std::move(processPath));
  }

  void pruneDeletedPaths(const Root& oldStorage, const Root& newStorage) {
    // This helper uses DeltaVisitor to visit all deleted nodes in
    // CHILDREN_FIRST order, and remove SubscriptionPathStores created
    // for corresponding paths.

    const auto oldRoot = oldStorage.root();
    const auto newRoot = newStorage.root();

    if (!oldRoot) {
      return;
    }

    auto processDelta = [&](CowDeletePathTraverseHelper& traverser,
                            auto& oldNode,
                            auto& newNode,
                            thrift_cow::DeltaElemTag visitTag) {
      bool isOldNode = static_cast<bool>(oldNode);
      bool isNewNode = static_cast<bool>(newNode);

      if (isOldNode && !isNewNode) {
        auto currStore = traverser.currentStore();
        if (currStore && (currStore->numSubsRecursive() == 0)) {
          auto parentStore = traverser.parentStore();
          if (parentStore) {
            auto path = traverser.path();
            auto lastTok = path.back();
            parentStore->removeChild(lastTok);
          }
        }
      }
    };

    CowDeletePathTraverseHelper traverser(&this->lookup_);

    thrift_cow::RootDeltaVisitor::visit(
        traverser,
        oldRoot,
        newRoot,
        thrift_cow::DeltaVisitOptions(
            thrift_cow::DeltaVisitMode::FULL,
            thrift_cow::DeltaVisitOrder::CHILDREN_FIRST,
            this->useIdPaths_),
        std::move(processDelta));
  }

  void serveSubscriptions(
      const Root& oldStorage,
      const Root& newStorage,
      const SubscriptionMetadataServer& metadataServer) {
    auto processChange = [&](CowSubscriptionTraverseHelper& traverser,
                             auto& oldNode,
                             auto& newNode,
                             thrift_cow::DeltaElemTag visitTag) {
      const auto* lookup = traverser.currentStore();
      if (!FLAGS_lazyPathStoreCreation) {
        // lookup can't be none or we wouldn't be serving this subscription
        DCHECK(lookup);
      } else {
        // with lazy path store creation, we may not have created the PathStore
        // yet. But there can still be subscriptions at parent nodes to serve.
        // So proceed with processing the change.
      }

      auto path = traverser.path();

      csm_detail::OperDeltaUnitCache deltaUnitCache(path, oldNode, newNode);

      if (lookup) {
        const auto& exactSubscriptions = lookup->subscriptions();
        for (auto& relevant : exactSubscriptions) {
          if (relevant->type() == PubSubType::PATH) {
            auto* pathSubscription =
                static_cast<BasePathSubscription*>(relevant);

            if (auto proto = pathSubscription->operProtocol(); proto) {
              servePathEncoded(
                  pathSubscription,
                  oldNode,
                  newNode,
                  *proto,
                  newStorage,
                  metadataServer);
            }
          } else {
            // serve delta subscriptions if it's a MINIMAL change
            // OR
            // if this is a fully added/removed node and there are exact
            // delta subscriptions, we serve them. This handles the case
            // where a change won't be "MINIMAL" relative to the root,
            // but is relative to the subscription point. This is mainly
            // for children of a fully added/removed node higher in the tree.
            if (visitTag == thrift_cow::DeltaElemTag::MINIMAL || !oldNode ||
                !newNode) {
              auto* deltaSubscription =
                  static_cast<DeltaSubscription*>(relevant);
              deltaSubscription->appendRootDeltaUnit(
                  deltaUnitCache.getEncoded(*relevant->operProtocol()));
            }
          }
        }
      }

      if (visitTag != thrift_cow::DeltaElemTag::MINIMAL) {
        // Done with path subs which need full traversal. Now only care about
        // MINIMAL changes for delta subs
        return;
      }

      // serve MINIMAL changes to delta subscription at parent paths
      const auto& traverseElements = traverser.elementsAlongPath();
      for (auto it = traverseElements.begin(); it != traverseElements.end() - 1;
           ++it) {
        if (!it->lookup) {
          // no path store for this path, so subscriptions to serve
          continue;
        }
        const auto& parentSubscriptions = it->lookup->subscriptions();
        for (auto& relevant : parentSubscriptions) {
          if (relevant->type() != PubSubType::DELTA) {
            continue;
          }
          auto* deltaSubscription = static_cast<DeltaSubscription*>(relevant);
          deltaSubscription->appendRootDeltaUnit(
              deltaUnitCache.getEncoded(*relevant->operProtocol()));
        }
      }
    };

    const auto& [oldRoot, newRoot] =
        std::tie(oldStorage.root(), newStorage.root());

    CowSubscriptionTraverseHelper traverser(&this->lookup_);
    if (oldRoot && newRoot) {
      thrift_cow::RootDeltaVisitor::visit(
          traverser,
          oldRoot,
          newRoot,
          thrift_cow::DeltaVisitOptions(
              thrift_cow::DeltaVisitMode::FULL,
              thrift_cow::DeltaVisitOrder::CHILDREN_FIRST,
              this->useIdPaths_),
          std::move(processChange));
    } else if (!oldRoot || !newRoot) {
      processChange(
          traverser, oldRoot, newRoot, thrift_cow::DeltaElemTag::MINIMAL);
    }

    this->flush(metadataServer);
  }

  void doInitialSync(
      const Root& newRoot,
      const SubscriptionMetadataServer& metadataServer) {
    /*
     * It is important we do the extended subscriptions before the
     * regular ones. Resolving the extended subscriptions could extend
     * the initial sync needed set based on the initial matching paths.
     */

    doInitialSyncExtended(newRoot, metadataServer);
    doInitialSyncSimple(newRoot, metadataServer);
    this->flush(metadataServer);
  }

}; // namespace facebook::fboss::fsdb

} // namespace facebook::fboss::fsdb
