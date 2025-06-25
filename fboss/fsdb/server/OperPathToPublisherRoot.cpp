// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/OperPathToPublisherRoot.h"
#include <folly/String.h>

namespace {
using facebook::fboss::fsdb::FsdbErrorCode;
using facebook::fboss::fsdb::FsdbException;

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

template <typename PathIterator>
void OperPathToPublisherRoot::checkNonEmpty(
    const PathIterator& begin,
    const PathIterator& end) const {
  auto it = begin;
  if (it != end) {
    if (rootPathLength_ > 1) {
      std::advance(it, rootPathLength_ - 1);
    }
  }
  if (it == end) {
    FsdbException e;
    e.message() = "Path empty or too short";
    e.errorCode() = FsdbErrorCode::INVALID_PATH;
    throw e;
  }
}

std::string OperPathToPublisherRoot::publisherRoot(PathIter begin, PathIter end)
    const {
  checkPath(begin, end);
  auto rootPathEnd = begin;
  std::advance(rootPathEnd, rootPathLength_);
  return folly::join('_', begin, rootPathEnd);
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
  if (rootPathLength_ > 1) {
    auto rootPathEnd = begin;
    std::advance(rootPathEnd, rootPathLength_);
    std::vector<std::string> elements;
    std::transform(
        begin, rootPathEnd, std::back_inserter(elements), [](const auto& elem) {
          return *elem.raw_ref();
        });
    return folly::join('_', elements);
  } else {
    return *begin->raw_ref();
  }
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
  checkNonEmpty(paths.begin(), paths.end());

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
  checkNonEmpty(operPathMap.begin(), operPathMap.end());

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
  checkNonEmpty(operPathMap.begin(), operPathMap.end());

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
