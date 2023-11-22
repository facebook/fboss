// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/Utils.h"
#include "fboss/fsdb/common/Utils.h"

namespace facebook::fboss::fsdb::test {

// static
Vectors Utils::makeVectors(const std::set<ExportTypeStr>& exportTypesStr) {
  const auto kVectorSize = fsdb::Utils::kDurationSuffixes.size();
  Vectors ret;
  auto val = 0;
  for (const auto& exportTypeAsStr : exportTypesStr) {
    std::vector<int64_t> v;
    v.reserve(kVectorSize);
    for (auto i = 0; i < kVectorSize; i++) {
      v.push_back(++val);
    }

    ret.insert({exportTypeAsStr, std::move(v)});
  }

  return ret;
}
} // namespace facebook::fboss::fsdb::test
