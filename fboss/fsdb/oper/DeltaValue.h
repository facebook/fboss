// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/gen-cpp2/patch_types.h"

namespace facebook::fboss::fsdb {

template <typename T>
struct DeltaValue {
  using value_type = T;

  DeltaValue() = default;
  DeltaValue(std::optional<T> oldVal, std::optional<T> newVal)
      : oldVal(std::move(oldVal)), newVal(std::move(newVal)) {}

  std::optional<T> oldVal;
  std::optional<T> newVal;
};

size_t getPatchNodeSize(const thrift_cow::PatchNode& val);

size_t getOperDeltaSize(const OperDelta& delta);

size_t getExtendedDeltaSize(const std::vector<TaggedOperDelta>& extDelta);

} // namespace facebook::fboss::fsdb
