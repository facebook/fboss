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

std::string multiPathMapStr(
    const std::map<SubscriptionKey, RawOperPath>& paths) {
  std::vector<std::string> pathStrs;
  for (const auto& [key, path] : paths) {
    pathStrs.push_back(folly::join("/", *path.path()));
  }
  return folly::join("_", pathStrs);
}

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
  if constexpr (std::is_same_v<Paths, std::vector<std::string>>) {
    return folly::join('_', path);
  } else if constexpr (std::is_same_v<Paths, std::vector<ExtendedOperPath>>) {
    return extendedPathsStr(path);
  } else {
    return multiPathMapStr(path);
  }
}

template class FsdbSubscriber<OperDelta, std::vector<std::string>>;
template class FsdbSubscriber<OperState, std::vector<std::string>>;
template class FsdbSubscriber<OperSubPathUnit, std::vector<ExtendedOperPath>>;
template class FsdbSubscriber<OperSubDeltaUnit, std::vector<ExtendedOperPath>>;
template class FsdbSubscriber<
    SubscriberChunk,
    std::map<SubscriptionKey, RawOperPath>>;

} // namespace facebook::fboss::fsdb
