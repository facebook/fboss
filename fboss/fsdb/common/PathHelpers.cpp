// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/common/PathHelpers.h"

#include <folly/String.h>

namespace facebook::fboss::fsdb {

std::string PathHelpers::toString(const std::vector<std::string>& path) {
  return folly::join("/", path);
}

std::string PathHelpers::toString(const RawOperPath& path) {
  return PathHelpers::toString(*path.path());
}

std::string PathHelpers::toString(const ExtendedOperPath& path) {
  std::vector<std::string> pathElms;
  for (const auto& pathElm : *path.path()) {
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
  return PathHelpers::toString(pathElms);
}

std::string PathHelpers::toString(const std::vector<ExtendedOperPath>& paths) {
  return folly::join("_", PathHelpers::toStringList(paths));
}

std::string PathHelpers::toString(
    const std::map<SubscriptionKey, RawOperPath>& paths) {
  return folly::join("_", PathHelpers::toStringList(paths));
}

std::vector<std::string> PathHelpers::toStringList(
    const std::map<SubscriptionKey, RawOperPath>& paths) {
  std::vector<std::string> strPaths;
  strPaths.reserve(paths.size());
  for (const auto& [_, path] : paths) {
    strPaths.push_back(PathHelpers::toString(path));
  }
  return strPaths;
}

std::vector<std::string> PathHelpers::toStringList(
    const std::vector<ExtendedOperPath>& paths) {
  std::vector<std::string> strPaths;
  strPaths.reserve(paths.size());
  for (const auto& path : paths) {
    strPaths.push_back(PathHelpers::toString(path));
  }
  return strPaths;
}

std::vector<std::string> PathHelpers::toStringList(
    const std::vector<std::string>& path) {
  return path;
}

std::vector<ExtendedOperPath> PathHelpers::toExtendedOperPath(
    const std::vector<std::vector<std::string>>& paths) {
  std::vector<ExtendedOperPath> extPaths;
  for (const auto& path : paths) {
    ExtendedOperPath extPath;
    extPath.path()->reserve(path.size());
    for (const auto& pathElm : path) {
      OperPathElem operPathElm;
      operPathElm.raw_ref() = pathElm;
      extPath.path()->push_back(std::move(operPathElm));
    }
    extPaths.push_back(std::move(extPath));
  }
  return extPaths;
}
std::map<SubscriptionKey, ExtendedOperPath> toMappedExtendedOperPath(
    const std::vector<std::vector<std::string>>& paths) {
  std::map<SubscriptionKey, ExtendedOperPath> result;
  for (size_t i = 0; i < paths.size(); i++) {
    auto path = paths[i];
    ExtendedOperPath extPath;
    extPath.path()->reserve(path.size());
    for (const auto& pathElm : path) {
      OperPathElem operPathElm;
      operPathElm.raw_ref() = pathElm;
      extPath.path()->push_back(std::move(operPathElm));
    }
    result[i] = std::move(extPath);
  }
  return result;
}

} // namespace facebook::fboss::fsdb
