// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename Paths>
SubscriptionType FsdbSubscriber<SubUnit, Paths>::subscriptionType() {
  if constexpr (
      std::is_same_v<SubUnit, OperDelta> ||
      std::is_same_v<SubUnit, OperSubDeltaUnit>) {
    return SubscriptionType::DELTA;
  } else if constexpr (
      std::is_same_v<SubUnit, OperState> ||
      std::is_same_v<SubUnit, OperSubPathUnit>) {
    return SubscriptionType::PATH;
  } else if constexpr (std::is_same_v<SubUnit, SubscriberChunk>) {
    return SubscriptionType::PATCH;
  } else {
    static_assert(folly::always_false<SubUnit>, "unsupported request type");
  }
}

template <typename SubUnit, typename Paths>
std::string FsdbSubscriber<SubUnit, Paths>::typeStr() const {
  auto subType = subscriptionType();
  return subscriptionTypeToStr[subType];
}
template <typename SubUnit, typename Paths>
std::string FsdbSubscriber<SubUnit, Paths>::pathsStr(const Paths& path) const {
  return PathHelpers::toString(path);
}

template class FsdbSubscriber<OperDelta, std::vector<std::string>>;
template class FsdbSubscriber<OperState, std::vector<std::string>>;
template class FsdbSubscriber<OperSubPathUnit, std::vector<ExtendedOperPath>>;
template class FsdbSubscriber<OperSubDeltaUnit, std::vector<ExtendedOperPath>>;
template class FsdbSubscriber<
    SubscriberChunk,
    std::map<SubscriptionKey, RawOperPath>>;

} // namespace facebook::fboss::fsdb
