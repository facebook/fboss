// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/Subscription.h"

#include <folly/CppAttributes.h>
#include <folly/FBString.h>
#include <folly/String.h>
#include <folly/container/F14Map.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <unordered_set>
#include <vector>

namespace facebook::fboss::fsdb {

DECLARE_bool(lazyPathStoreCreation);

enum class LookupType { PARENTS, TARGET, CHILDREN };

struct PartiallyResolvedExtendedSubscription {
  std::weak_ptr<ExtendedSubscription> subscription;
  SubscriptionKey subKey{0};
  uint16_t elemIdx{0};
  std::unordered_set<std::string> previouslyResolved{};

  void debugPrint() const;
};

class SubscriptionStore;

struct SubscriptionPathStoreTreeStats {
  uint64_t numPathStores{0};
  uint64_t numPathStoreAllocs{0};
  uint64_t numPathStoreFrees{0};
};

class SubscriptionPathStore {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  explicit SubscriptionPathStore(SubscriptionPathStoreTreeStats* stats);

  void add(Subscription* subscription, SubscriptionPathStoreTreeStats* stats);

  void remove(Subscription* subscription);

  void incrementallyResolve(
      SubscriptionStore& store,
      std::shared_ptr<ExtendedSubscription> subscription,
      SubscriptionKey subKey,
      std::vector<std::string>& pathSoFar);

  void processAddedPath(
      SubscriptionStore& store,
      PathIter begin,
      PathIter curr,
      PathIter end);

  std::vector<Subscription*> find(
      PathIter begin,
      PathIter end,
      std::unordered_set<LookupType> lookupTypes);

  SubscriptionPathStore* FOLLY_NULLABLE child(const std::string& key) const;

  SubscriptionPathStore* getOrCreateChild(
      const std::string& key,
      SubscriptionPathStoreTreeStats* stats);

  void removeChild(
      const std::string& key,
      SubscriptionPathStoreTreeStats* stats);

  void clear(SubscriptionPathStoreTreeStats* stats);

  void debugPrint() const;

  uint32_t numSubs() const {
    return numSubs_;
  }
  uint32_t numChildSubs() const {
    return numChildSubs_;
  }
  uint32_t numSubsRecursive() const {
    return numSubs() + numChildSubs();
  }
  uint32_t numPathStoresRecursive_Expensive() const {
    uint32_t totalPathStores{1};
    for (auto& [_name, child] : children_) {
      totalPathStores += child->numPathStoresRecursive_Expensive();
    }
    return totalPathStores;
  }

  const SubscriptionPathStore* FOLLY_NULLABLE
  findStore(PathIter begin, PathIter end) const;

  const std::vector<Subscription*>& subscriptions() const {
    return subscriptions_;
  }

  std::vector<Subscription*>& subscriptions() {
    return subscriptions_;
  }

 private:
  void freePathStore(SubscriptionPathStoreTreeStats* stats);

  void debugPrint(std::vector<std::string>& pathSoFar) const;

  void gatherChildren(std::vector<Subscription*>& gathered);

  void add(
      SubscriptionPathStoreTreeStats* stats,
      PathIter begin,
      PathIter end,
      Subscription* subscription);

  // returns true if a subscription was actually removed
  bool remove(PathIter begin, PathIter end, Subscription* subscription);

  void incrementCounts(Subscription* subscription, bool isChild);
  void decrementCounts(Subscription* subscription, bool isChild);

  folly::F14FastMap<folly::fbstring, std::shared_ptr<SubscriptionPathStore>>
      children_;
  std::vector<Subscription*> subscriptions_;
  std::vector<PartiallyResolvedExtendedSubscription> partiallyResolvedSubs_;

  // as we add to the store, we keep a count of children subscriptions by type
  // for fast lookup this will track count of subscriptions at this path and at
  // children
  uint32_t numSubs_{0};
  uint32_t numChildSubs_{0};
};

} // namespace facebook::fboss::fsdb
