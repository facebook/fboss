// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_types.h"

namespace facebook::fboss::fsdb::test {

class Utils {
 public:
  static Vectors makeVectors(const std::set<ExportTypeStr>& exportTypesStr);
};

} // namespace facebook::fboss::fsdb::test
