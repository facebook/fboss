// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/OperPathToPublisherRoot.h"

namespace {
using facebook::fboss::fsdb::FsdbErrorCode;
using facebook::fboss::fsdb::FsdbException;

template <typename PathIter>
void checkNonEmpty(const PathIter& begin, const PathIter& end) {
  if (begin == end) {
    FsdbException e;
    e.message() = "Empty path";
    e.errorCode() = FsdbErrorCode::INVALID_PATH;
    throw e;
  }
}

template <typename Path>
void checkNonEmpty(const Path& path) {
  checkNonEmpty(path.begin(), path.end());
}

void checkCrossRoot(const std::string& root1, const std::string& root2) {
  if (root1 != root2) {
    FsdbException e;
    e.message() = fmt::format(
        "Subscription spans multiple publisher roots: {} and {}", root1, root2);
    e.errorCode() = FsdbErrorCode::INVALID_PATH;
    throw e;
  }
}
} // namespace

namespace facebook::fboss::fsdb {

std::string OperPathToPublisherRoot::publisherRoot(PathIter begin, PathIter end)
    const {
  checkPath(begin, end);
  return *begin;
}

std::string OperPathToPublisherRoot::publisherRoot(const Path& path) const {
  return publisherRoot(path.begin(), path.end());
}

std::string OperPathToPublisherRoot::publisherRoot(const OperPath& path) const {
  return publisherRoot(*path.raw());
}

std::string OperPathToPublisherRoot::publisherRoot(
    const RawOperPath& operPath) const {
  return publisherRoot(*operPath.path());
}

std::string OperPathToPublisherRoot::publisherRoot(
    ExtPathIter begin,
    ExtPathIter end) const {
  checkExtendedPath(begin, end);
  return *begin->raw_ref();
}

std::string OperPathToPublisherRoot::publisherRoot(const ExtPath& path) const {
  return publisherRoot(path.begin(), path.end());
}

std::string OperPathToPublisherRoot::publisherRoot(
    const ExtendedOperPath& path) const {
  return publisherRoot(*path.path());
}

std::string OperPathToPublisherRoot::publisherRoot(
    const std::vector<ExtendedOperPath>& paths) const {
  checkNonEmpty(paths);

  std::optional<std::string> root;
  for (const auto& path : paths) {
    auto currRoot = publisherRoot(path);
    if (root) {
      checkCrossRoot(*root, currRoot);
    } else {
      root = currRoot;
    }
  }

  return root.value();
}

std::string OperPathToPublisherRoot::publisherRoot(
    const RawSubPathMap& operPathMap) const {
  checkNonEmpty(operPathMap);

  std::optional<std::string> root;
  for (const auto& [_, path] : operPathMap) {
    auto currRoot = publisherRoot(path);
    if (root) {
      checkCrossRoot(*root, currRoot);
    } else {
      root = currRoot;
    }
  }

  return root.value();
}

std::string OperPathToPublisherRoot::publisherRoot(
    const ExtSubPathMap& operPathMap) const {
  checkNonEmpty(operPathMap);

  std::optional<std::string> root;
  for (const auto& [_, path] : operPathMap) {
    auto currRoot = publisherRoot(path);
    if (root) {
      checkCrossRoot(*root, currRoot);
    } else {
      root = currRoot;
    }
  }

  return root.value();
}

void OperPathToPublisherRoot::checkPath(PathIter begin, PathIter end) const {
  checkNonEmpty(begin, end);
}

void OperPathToPublisherRoot::checkExtendedPath(
    ExtPathIter begin,
    ExtPathIter end) const {
  checkNonEmpty(begin, end);

  auto& elem = *begin;
  if (elem.getType() != OperPathElem::Type::raw) {
    FsdbException e;
    e.message() =
        "Cannot support wildcard types as first element of extended path";
    e.errorCode() = FsdbErrorCode::INVALID_PATH;
    throw e;
  }
}

} // namespace facebook::fboss::fsdb
