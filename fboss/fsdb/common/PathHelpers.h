// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

class PathHelpers {
 public:
  static std::string toString(const std::vector<std::string>& path);
  static std::string toString(const RawOperPath& path);
  static std::string toString(const ExtendedOperPath& path);
  static std::string toString(const std::vector<ExtendedOperPath>& paths);
  static std::string toString(
      const std::map<SubscriptionKey, RawOperPath>& paths);

  static std::vector<std::string> toStringList(
      const std::map<SubscriptionKey, RawOperPath>& paths);
  static std::vector<std::string> toStringList(
      const std::vector<ExtendedOperPath>& paths);
  static std::vector<std::string> toStringList(
      const std::vector<std::string>& path);

  static std::vector<ExtendedOperPath> toExtendedOperPath(
      const std::vector<std::vector<std::string>>& paths);
  static std::map<SubscriptionKey, ExtendedOperPath> toMappedExtendedOperPath(
      const std::vector<std::vector<std::string>>& paths);
};

} // namespace facebook::fboss::fsdb
