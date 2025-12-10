// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/coro/AsyncGenerator.h>
#include <cstddef>

namespace facebook::fboss::fsdb {

// shared object to keep track of subscription stream metrics
struct SubscriptionStreamInfo {
  std::atomic<uint64_t> enqueuedDataSize{0};
  std::atomic<uint64_t> servedDataSize{0};
};

template <typename generator_type>
struct SubscriptionStreamReader {
  folly::coro::AsyncGenerator<generator_type&&> generator_;
  std::shared_ptr<SubscriptionStreamInfo> streamInfo_;
};

// Wrapper type for subscription serve queue elements
template <typename T>
struct SubscriptionServeQueueElement {
  T val;
  size_t allocatedBytes{0};

  explicit SubscriptionServeQueueElement(T&& v, size_t bytes = 0)
      : val(std::move(v)), allocatedBytes(bytes) {}
};

} // namespace facebook::fboss::fsdb
