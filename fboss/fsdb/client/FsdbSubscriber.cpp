// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename PathElement>
std::string FsdbSubscriber<SubUnit, PathElement>::typeStr() const {
  return std::disjunction_v<
             std::is_same<SubUnit, OperDelta>,
             std::is_same<SubUnit, TaggedOperDelta>>
      ? "Delta"
      : "Path";
}
template <typename SubUnit, typename PathElement>
std::string FsdbSubscriber<SubUnit, PathElement>::pathStr(
    const Paths& path) const {
  if constexpr (std::is_same_v<PathElement, std::string>) {
    return folly::join('_', path);
  } else {
    // TODO
    return "";
  }
}

template class FsdbSubscriber<OperDelta, std::string>;
template class FsdbSubscriber<OperState, std::string>;
template class FsdbSubscriber<TaggedOperDelta, ExtendedOperPath>;
template class FsdbSubscriber<TaggedOperState, ExtendedOperPath>;

} // namespace facebook::fboss::fsdb
