// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/Subscription.h"

#include <folly/CppAttributes.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace facebook::fboss::fsdb {

enum class LookupType { PARENTS, TARGET, CHILDREN };

struct PartiallyResolvedExtendedSubscription {
  std::weak_ptr<ExtendedSubscription> subscription;
  std::size_t pathIdx{0};
  uint16_t elemIdx{0};
  std::unordered_set<std::string> previouslyResolved{};

  void debugPrint() const;
};

class SubscriptionManagerBase;

class SubscriptionPathStore {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  void add(Subscription* subscription);

  void remove(Subscription* subscription);

  void incrementallyResolve(
      SubscriptionManagerBase& manager,
      std::shared_ptr<ExtendedSubscription> subscription,
      std::size_t pathIdx,
      std::vector<std::string>& pathSoFar);

  void processAddedPath(
      SubscriptionManagerBase& manager,
      PathIter begin,
      PathIter curr,
      PathIter end);

  std::vector<Subscription*> find(
      PathIter begin,
      PathIter end,
      std::unordered_set<LookupType> lookupTypes);

  SubscriptionPathStore* FOLLY_NULLABLE child(const std::string& key) const;

  SubscriptionPathStore* getOrCreateChild(const std::string& key);

  void removeChild(const std::string& key);

  void debugPrint() const;

  uint32_t numSubs() const {
    return numDeltaSubs_ + numPathSubs_;
  }
  uint32_t numChildSubs() const {
    return numChildDeltaSubs_ + numChildPathSubs_;
  }
  uint32_t numSubsRecursive() const {
    return numSubs() + numChildSubs();
  }
  uint32_t numDeltaSubs() const {
    return numDeltaSubs_;
  }
  uint32_t numPathSubs() const {
    return numPathSubs_;
  }
  uint32_t numChildDeltaSubs() const {
    return numChildDeltaSubs_;
  }
  uint32_t numChildPathSubs() const {
    return numChildPathSubs_;
  }
  uint32_t numPathStores() const {
    auto totalPathStores = children_.size();
    for (auto& [_name, child] : children_) {
      totalPathStores += child->numPathStores();
    }
    return totalPathStores;
  }

  const SubscriptionPathStore* FOLLY_NULLABLE
  findStore(PathIter begin, PathIter end) const;

  const std::vector<Subscription*>& subscriptions() const {
    return subscriptions_;
  }

 private:
  void debugPrint(std::vector<std::string>& pathSoFar) const;

  void gatherChildren(std::vector<Subscription*>& gathered);

  void add(PathIter begin, PathIter end, Subscription* subscription);

  // returns true if a subscription was actually removed
  bool remove(PathIter begin, PathIter end, Subscription* subscription);

  void incrementCounts(Subscription* subscription, bool isChild);
  void decrementCounts(Subscription* subscription, bool isChild);

  std::unordered_map<std::string, std::shared_ptr<SubscriptionPathStore>>
      children_;
  std::vector<Subscription*> subscriptions_;
  std::vector<PartiallyResolvedExtendedSubscription> partiallyResolvedSubs_;

  // as we add to the store, we keep a count of children subscriptions by type
  // for fast lookup this will track count of subscriptions at this path and at
  // children
  uint32_t numDeltaSubs_{0};
  uint32_t numPathSubs_{0};
  uint32_t numChildDeltaSubs_{0};
  uint32_t numChildPathSubs_{0};
};

} // namespace facebook::fboss::fsdb
