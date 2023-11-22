/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_config_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

class PathValidator {
 public:
  // will raise if invalid path
  static void validateStatePath(const std::vector<std::string>& path);
  static void validateStatsPath(const std::vector<std::string>& path);
  static void validateExtendedStatePath(const ExtendedOperPath& path);
  static void validateExtendedStatsPath(const ExtendedOperPath& path);
  static void validateExtendedStatePaths(
      const std::vector<ExtendedOperPath>& paths);
  static void validateExtendedStatsPaths(
      const std::vector<ExtendedOperPath>& paths);
  static bool isStatePathValid(const std::vector<std::string>& path);
  static bool isStatsPathValid(const std::vector<std::string>& path);
};

} // namespace facebook::fboss::fsdb
