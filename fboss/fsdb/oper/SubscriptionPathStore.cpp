// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionPathStore.h"
#include "fboss/fsdb/oper/SubscriptionManager.h"

namespace facebook::fboss::fsdb {

void SubscriptionPathStore::add(Subscription* subscription) {
  auto path = subscription->path();
  add(path.begin(), path.end(), subscription);
}

void SubscriptionPathStore::remove(Subscription* subscription) {
  auto path = subscription->path();
  remove(path.begin(), path.end(), subscription);
}

std::vector<Subscription*> SubscriptionPathStore::find(
    PathIter begin,
    PathIter end,
    std::unordered_set<LookupType> lookupTypes) {
  if (lookupTypes.empty()) {
    return {};
  }

  std::vector<Subscription*> gathered;

  if (begin == end) {
    if (lookupTypes.count(LookupType::TARGET)) {
      std::copy(
          subscriptions_.begin(),
          subscriptions_.end(),
          std::back_inserter(gathered));
    }

    if (lookupTypes.count(LookupType::CHILDREN)) {
      for (auto& [_name, child] : children_) {
        child->gatherChildren(gathered);
      }
    }
  } else {
    if (lookupTypes.count(LookupType::PARENTS)) {
      std::copy(
          subscriptions_.begin(),
          subscriptions_.end(),
          std::back_inserter(gathered));
    }

    auto key = *begin++;
    if (auto it = children_.find(key); it != children_.end()) {
      auto toAppend = it->second->find(begin, end, std::move(lookupTypes));
      std::copy(toAppend.begin(), toAppend.end(), std::back_inserter(gathered));
    }
  }

  return gathered;
}

SubscriptionPathStore* FOLLY_NULLABLE
SubscriptionPathStore::child(const std::string& key) const {
  if (auto it = children_.find(key); it != children_.end()) {
    return it->second.get();
  } else {
    return nullptr;
  }
}

SubscriptionPathStore* SubscriptionPathStore::getOrCreateChild(
    const std::string& key) {
  auto [it, added] =
      children_.emplace(key, std::make_shared<SubscriptionPathStore>());
  return it->second.get();
}

void SubscriptionPathStore::removeChild(const std::string& key) {
  if (auto it = children_.find(key); it == children_.end()) {
    return;
  }
  auto child = children_[key];
  CHECK_EQ(children_[key]->numSubsRecursive(), 0);
  children_.erase(key);
  return;
}

void SubscriptionPathStore::gatherChildren(
    std::vector<Subscription*>& gathered) {
  std::copy(
      subscriptions_.begin(),
      subscriptions_.end(),
      std::back_inserter(gathered));

  for (auto& [_name, child] : children_) {
    child->gatherChildren(gathered);
  }
}

void SubscriptionPathStore::add(
    PathIter begin,
    PathIter end,
    Subscription* subscription) {
  if (begin == end) {
    subscriptions_.push_back(subscription);
    incrementCounts(subscription, false);
    return;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it == children_.end()) {
    children_.emplace(key, std::make_shared<SubscriptionPathStore>());
  }

  children_[key]->add(begin, end, subscription);
  incrementCounts(subscription, true);
}

bool SubscriptionPathStore::remove(
    PathIter begin,
    PathIter end,
    Subscription* subscription) {
  if (begin == end) {
    auto it =
        std::find(subscriptions_.begin(), subscriptions_.end(), subscription);
    if (it != subscriptions_.end()) {
      subscriptions_.erase(it);
      decrementCounts(subscription, false);
      return true;
    }
    return false;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it == children_.end()) {
    return false;
  }

  if (children_[key]->remove(begin, end, subscription)) {
    decrementCounts(subscription, true);
    return true;
  }

  return false;
}

void SubscriptionPathStore::incrementCounts(
    Subscription* subscription,
    bool isChild) {
  switch (subscription->type()) {
    case PubSubType::PATH:
      if (isChild) {
        ++numChildPathSubs_;
      } else {
        ++numPathSubs_;
      }
      return;
    case PubSubType::DELTA:
      if (isChild) {
        ++numChildDeltaSubs_;
      } else {
        ++numDeltaSubs_;
      }
      return;
  }
}

void SubscriptionPathStore::decrementCounts(
    Subscription* subscription,
    bool isChild) {
  switch (subscription->type()) {
    case PubSubType::PATH:
      if (isChild) {
        --numChildPathSubs_;
      } else {
        --numPathSubs_;
      }
      return;
    case PubSubType::DELTA:
      if (isChild) {
        --numChildDeltaSubs_;
      } else {
        --numDeltaSubs_;
      }
      return;
  }
}

const SubscriptionPathStore* FOLLY_NULLABLE
SubscriptionPathStore::findStore(PathIter begin, PathIter end) const {
  if (begin == end) {
    return this;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it != children_.end()) {
    return it->second->findStore(begin, end);
  }

  return nullptr;
}

void SubscriptionPathStore::debugPrint() const {
  std::vector<std::string> emptyPathSoFar;
  debugPrint(emptyPathSoFar);
}

void SubscriptionPathStore::debugPrint(
    std::vector<std::string>& pathSoFar) const {
  XLOG(INFO);
  XLOG(INFO) << "SubscriptionPathStore at " << '/'
             << folly::join('/', pathSoFar);
  XLOG(INFO) << "children_.size()=" << children_.size();
  XLOG(INFO) << "partiallyResolvedSubs_.size()="
             << partiallyResolvedSubs_.size();
  if (!children_.empty()) {
    std::vector<std::string> children;
    for (const auto& [child, _] : children_) {
      children.emplace_back(child);
    }
    XLOG(INFO) << "Children: " << folly::join(',', children);
  }
  if (!partiallyResolvedSubs_.empty()) {
    XLOG(INFO) << "Partially Resolved: ";
    for (const auto& partial : partiallyResolvedSubs_) {
      partial.debugPrint();
    }
  }
  for (const auto& [child, childStore] : children_) {
    pathSoFar.emplace_back(child);
    childStore->debugPrint(pathSoFar);
    pathSoFar.pop_back();
  }
}

void PartiallyResolvedExtendedSubscription::debugPrint() const {
  XLOG(INFO) << "\tsubscription=" << subscription.lock().get()
             << ", pathIdx=" << pathIdx << ", elemIdx=" << elemIdx
             << ", previouslyResolved=" << folly::join(',', previouslyResolved);
}

} // namespace facebook::fboss::fsdb
