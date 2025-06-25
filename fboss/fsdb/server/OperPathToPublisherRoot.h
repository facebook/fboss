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
  using RawSubPathMap = std::map<SubscriptionKey, RawOperPath>;
  using ExtSubPathMap = std::map<SubscriptionKey, ExtendedOperPath>;

  OperPathToPublisherRoot(unsigned int rootPathLength = 1)
      : rootPathLength_(rootPathLength) {}
  /*
   * We use the conventions of publisher root paths are of format
   * {optional_root_prefix_path[], publisherRoot}
   *
   * For FSDB data model paths are of form {publisherRoot}
   * i.e. rootPathLength = 1
   * In future, if we ever change this, we could create a
   * mapping from (sub) path to a publisher root. So
   * for e.g. {"unit0","agent"} may map to publish root
   * of unit0_agent
   *
   * In case of NSDB data model where paths are of form /devices/<deviceName>
   * using rootPathLength = 2 allows each deviceName to be mapped
   * to publisher root of devices_{deviceName}.
   */
  std::string publisherRoot(PathIter begin, PathIter end) const;

  std::string publisherRoot(const RawOperPath& operPath) const;

  std::string publisherRoot(const Path& path) const;

  std::string publisherRoot(const OperPath& path) const;

  std::string publisherRoot(ExtPathIter begin, ExtPathIter end) const;

  std::string publisherRoot(const ExtPath& path) const;

  std::string publisherRoot(const ExtendedOperPath& path) const;

  std::string publisherRoot(const std::vector<ExtendedOperPath>& paths) const;

  std::string publisherRoot(const RawSubPathMap& operPathMap) const;

  std::string publisherRoot(const ExtSubPathMap& operPathMap) const;

 private:
  unsigned int rootPathLength_;

  template <typename PathIterator>
  void checkNonEmpty(const PathIterator& begin, const PathIterator& end) const;

  void checkPath(PathIter begin, PathIter end) const;

  void checkExtendedPath(ExtPathIter begin, ExtPathIter end) const;
};
} // namespace facebook::fboss::fsdb
