// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

class OperPathToPublisherRoot {
 public:
  using Path = std::vector<std::string>;
  using PathItr = Path::const_iterator;

  /* We use the conventions of taking the first element of
   * path and considering it as publisher root. This matches
   * the structure of our FSDB tree. However in the
   * future, if we ever change this, we could create a
   * mapping from (sub) path to a publisher root. So
   * for e.g. {"unit0","agent"} may map to publish root
   * of unit0_agent
   */
  std::string publisherRoot(PathItr beg, PathItr end) const {
    checkPath(beg, end);
    return *beg;
  }
  std::string publisherRoot(const Path& path) const {
    return publisherRoot(path.begin(), path.end());
  }
  std::string publisherRoot(const OperPath& path) const {
    return publisherRoot(*path.raw());
  }

 private:
  void checkPath(PathItr begin, PathItr end) const {
    if (begin == end) {
      FsdbException e;
      e.message_ref() = "Empty path";
      e.errorCode_ref() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }
  }
};
} // namespace facebook::fboss::fsdb
