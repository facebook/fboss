// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

class OperPathToPublisherRoot {
 public:
  // TODO: should we move this logic in to the higher PathValidator
  // layer?

  using Path = std::vector<std::string>;
  using PathIter = Path::const_iterator;
  using ExtPath = std::vector<OperPathElem>;
  using ExtPathIter = ExtPath::const_iterator;

  /* We use the conventions of taking the first element of
   * path and considering it as publisher root. This matches
   * the structure of our FSDB tree. However in the
   * future, if we ever change this, we could create a
   * mapping from (sub) path to a publisher root. So
   * for e.g. {"unit0","agent"} may map to publish root
   * of unit0_agent
   */
  std::string publisherRoot(PathIter begin, PathIter end) const {
    checkPath(begin, end);
    return *begin;
  }

  std::string publisherRoot(const Path& path) const {
    return publisherRoot(path.begin(), path.end());
  }

  std::string publisherRoot(const OperPath& path) const {
    return publisherRoot(*path.raw());
  }

  std::string publisherRoot(ExtPathIter begin, ExtPathIter end) const {
    checkExtendedPath(begin, end);
    return *begin->raw_ref();
  }

  std::string publisherRoot(const ExtPath& path) const {
    return publisherRoot(path.begin(), path.end());
  }

  std::string publisherRoot(const ExtendedOperPath& path) const {
    return publisherRoot(*path.path());
  }

  std::string publisherRoot(const std::vector<ExtendedOperPath>& paths) const {
    if (paths.size() == 0) {
      FsdbException e;
      e.message() = "Empty path";
      e.errorCode() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }

    std::optional<std::string> root;
    for (const auto& path : paths) {
      auto currRoot = publisherRoot(path);
      if (root) {
        if (*root != currRoot) {
          FsdbException e;
          e.message() = "Extended subscription spans multiple publisher roots";
          e.errorCode() = FsdbErrorCode::INVALID_PATH;
          throw e;
        }
      } else {
        root = currRoot;
      }
    }

    return *root;
  }

 private:
  void checkPath(PathIter begin, PathIter end) const {
    if (begin == end) {
      FsdbException e;
      e.message() = "Empty path";
      e.errorCode() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }
  }

  void checkExtendedPath(ExtPathIter begin, ExtPathIter end) const {
    if (begin == end) {
      FsdbException e;
      e.message() = "Empty path";
      e.errorCode() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }

    auto& elem = *begin;
    if (elem.getType() != OperPathElem::Type::raw) {
      FsdbException e;
      e.message() =
          "Cannot support wildcard types as first element of extended path";
      e.errorCode() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }
  }
};
} // namespace facebook::fboss::fsdb
