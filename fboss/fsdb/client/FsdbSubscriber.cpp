// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

namespace facebook::fboss::fsdb {

std::string extendedPathsStr(const std::vector<ExtendedOperPath>& extPaths) {
  std::vector<std::string> pathStrs;
  for (const auto& extPath : extPaths) {
    std::vector<std::string> pathElms;
    for (const auto& pathElm : *extPath.path()) {
      switch (pathElm.getType()) {
        case OperPathElem::Type::raw:
          pathElms.push_back(pathElm.raw_ref().value());
          break;
        case OperPathElem::Type::regex:
          pathElms.push_back(pathElm.regex_ref().value());
          break;
        case OperPathElem::Type::any:
          pathElms.push_back("*");
          break;
        case OperPathElem::Type::__EMPTY__:
          throw std::runtime_error("Illformed extended path");
          break;
      }
    }
    pathStrs.push_back(folly::join("/", pathElms));
  }
  return folly::join("_", pathStrs);
}
template <typename SubUnit, typename PathElement>
std::string FsdbSubscriber<SubUnit, PathElement>::typeStr() const {
  return std::disjunction_v<
             std::is_same<SubUnit, OperDelta>,
             std::is_same<SubUnit, TaggedOperDelta>>
      ? "Delta"
      : "Path";
}
template <typename SubUnit, typename PathElement>
std::string FsdbSubscriber<SubUnit, PathElement>::pathsStr(
    const Paths& path) const {
  if constexpr (std::is_same_v<PathElement, std::string>) {
    return folly::join('_', path);
  } else {
    return extendedPathsStr(path);
  }
}

template <typename SubUnit, typename PathElement>
void FsdbSubscriber<SubUnit, PathElement>::bumpServerLiveness() {
  auto now = std::chrono::system_clock::now();
  lastServerAliveTs_ =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
}

template class FsdbSubscriber<OperDelta, std::string>;
template class FsdbSubscriber<OperState, std::string>;
template class FsdbSubscriber<OperSubPathUnit, ExtendedOperPath>;
template class FsdbSubscriber<OperSubDeltaUnit, ExtendedOperPath>;

} // namespace facebook::fboss::fsdb
