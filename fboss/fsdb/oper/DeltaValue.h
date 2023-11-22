// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <optional>

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

} // namespace facebook::fboss::fsdb
