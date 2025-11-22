// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstddef>

namespace facebook::fboss::fsdb {

// Wrapper type for subscription serve queue elements
template <typename T>
struct SubscriptionServeQueueElement {
  T val;
  size_t allocatedBytes{0};

  explicit SubscriptionServeQueueElement(T&& v, size_t bytes = 0)
      : val(std::move(v)), allocatedBytes(bytes) {}
};

} // namespace facebook::fboss::fsdb
