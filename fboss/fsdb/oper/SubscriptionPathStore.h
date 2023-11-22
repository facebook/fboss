// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <re2/re2.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/CppAttributes.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include "fboss/fsdb/oper/Subscription.h"

namespace facebook::fboss::fsdb {

enum class LookupType { PARENTS, TARGET, CHILDREN };

struct PartiallyResolvedExtendedSubscription {
  std::weak_ptr<ExtendedSubscription> subscription;
  std::size_t pathIdx{0};
  uint16_t elemIdx{0};
  std::unordered_set<std::string> previouslyResolved{};

  void debugPrint() const;
};

class SubscriptionPathStore {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  void add(Subscription* subscription);

  void remove(Subscription* subscription);

  template <typename Manager>
  void incrementallyResolve(
      Manager& manager,
      std::shared_ptr<ExtendedSubscription> subscription,
      std::size_t pathIdx,
      std::vector<std::string>& pathSoFar) {
    // This incrementally extends our subscription tree based on an extended
    // path. Logic is to build up tree until next wildcard, or end of the path.

    const auto& path = subscription->pathAt(pathIdx).path().value();
    auto idx = pathSoFar.size();

    if (idx == path.size()) {
      // have reached the end of the extended path, add a
      // resolved Subscription*
      XLOG(DBG2) << this << ": Saving fully resolved extended subscription at "
                 << folly::join('/', pathSoFar);
      auto resolvedSub = subscription->resolve(pathSoFar);
      manager.registerSubscription(std::move(resolvedSub));
      return;
    } else if (path.at(idx).getType() != OperPathElem::Type::raw) {
      // we hit another wildcard element. Add a partially resolved subscription
      PartiallyResolvedExtendedSubscription partial;
      partial.subscription = subscription;
      partial.pathIdx = pathIdx;
      partial.elemIdx = idx;

      XLOG(DBG2) << this << ": Saving partially resolved subscription at "
                 << folly::join('/', pathSoFar);
      partiallyResolvedSubs_.emplace_back(std::move(partial));

      return;
    }

    const auto& key = path.at(idx).raw_ref().value();
    if (auto it = children_.find(key); it == children_.end()) {
      children_.emplace(key, std::make_shared<SubscriptionPathStore>());
    }
    pathSoFar.emplace_back(key);
    children_.at(key)->incrementallyResolve(
        manager, std::move(subscription), pathIdx, pathSoFar);
    pathSoFar.pop_back();
  }

  template <typename Manager>
  void processAddedPath(
      Manager& manager,
      PathIter begin,
      PathIter curr,
      PathIter end) {
    // this takes in a newly added path and then ensures that we
    // resolve any relevant partially resolved subscriptions

    if (curr == end) {
      return;
    }

    auto key = *curr++;
    if (auto it = children_.find(key); it == children_.end()) {
      children_.emplace(key, std::make_shared<SubscriptionPathStore>());
    }

    auto it = partiallyResolvedSubs_.begin();
    while (it != partiallyResolvedSubs_.end()) {
      auto& partial = *it;

      auto subscription = partial.subscription.lock();
      if (!subscription) {
        XLOG(DBG2) << this << ": Removing defunct partial subscription";
        it = partiallyResolvedSubs_.erase(it);
        continue;
      }

      if (auto findIt = partial.previouslyResolved.find(key);
          findIt != partial.previouslyResolved.end()) {
        ++it;
        continue;
      }
      partial.previouslyResolved.insert(key);

      const auto& path = subscription->pathAt(partial.pathIdx).path().value();
      const auto& elem = path.at(partial.elemIdx);
      bool matches{false};

      if (elem.getType() == OperPathElem::Type::any) {
        matches = true;
      } else if (auto regexStr = elem.regex_ref()) {
        matches = re2::RE2::FullMatch(key, *regexStr);
      }

      if (matches) {
        // we match this wildcard, incrementally resolve the
        // partial subscription to next wilcard or the end.
        std::vector<std::string> pathSoFar(begin, curr);
        children_.at(key)->incrementallyResolve(
            manager, std::move(subscription), partial.pathIdx, pathSoFar);
      }

      ++it;
    }

    children_.at(key)->processAddedPath(manager, begin, curr, end);
  }

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
