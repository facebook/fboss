// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"
#include "fboss/fsdb/common/PathHelpers.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename Paths>
std::string FsdbSubscriber<SubUnit, Paths>::typeStr() const {
  if constexpr (
      std::is_same_v<SubUnit, OperDelta> ||
      std::is_same_v<SubUnit, TaggedOperDelta>) {
    return "Delta";
  } else if (
      std::is_same_v<SubUnit, OperState> ||
      std::is_same_v<SubUnit, TaggedOperState>) {
    return "Path";
  } else {
    return "Patch";
  }
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
