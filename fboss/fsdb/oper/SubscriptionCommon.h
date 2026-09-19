// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

#include <folly/hash/Hash.h>

#include <algorithm>
#include <cstdint>

namespace facebook::fboss::fsdb {

// Serve intervals are quantized to whole ticks: a requested interval is rounded
// UP to the next tick multiple and clamped to the storage's default interval,
// which bounds the bucket space at defaultIntervalMs / tickMs.
// Widened so the caller's seconds-to-ms conversion cannot overflow; the clamp
// to maxMs below is the only bound a request needs.
constexpr uint32_t normalizeServeIntervalMs(
    uint64_t requestedMs,
    uint32_t tickMs,
    uint32_t maxMs) {
  if (tickMs == 0) {
    return maxMs;
  }
  const uint64_t rounded = ((requestedMs + tickMs - 1) / tickMs) * tickMs;
  return static_cast<uint32_t>(std::clamp<uint64_t>(rounded, tickMs, maxMs));
}

// Bucket 0 is served every tick; the last bucket is the default interval.
constexpr size_t serveBucketIndex(uint32_t intervalMs, uint32_t tickMs) {
  return tickMs == 0 ? 0 : (intervalMs / tickMs) - 1;
}

constexpr size_t serveBucketCount(uint32_t tickMs, uint32_t maxMs) {
  return tickMs == 0 ? 1 : maxMs / tickMs;
}

// Bounded to the configuration that has actually been validated; each occupied
// bucket also retains its own tree baseline.
inline constexpr size_t kMaxServeBuckets{5};

// SubscriptionIdentifier: helper to facilitate referencing a
// subscription by a unique identifier or alternate key.
class SubscriptionIdentifier {
 public:
  explicit SubscriptionIdentifier(
      const SubscriberId& subscriberId,
      uint64_t uid = 0)
      : subscriberId_(subscriberId), uid_(uid) {}

  explicit SubscriptionIdentifier(const ClientId& clientId, uint64_t uid = 0)
      : subscriberId_(clientId2SubscriberId(clientId)), uid_(uid) {}

  SubscriptionIdentifier(const SubscriptionIdentifier& other)
      : subscriberId_(other.subscriberId_), uid_(other.uid()) {}

  SubscriptionIdentifier(SubscriptionIdentifier&& other) noexcept
      : subscriberId_(std::move(other.subscriberId_)), uid_(other.uid()) {}

  const SubscriberId& subscriberId() const {
    return subscriberId_;
  }
  uint64_t uid() const {
    return uid_;
  }

  bool operator==(const SubscriptionIdentifier& other) const {
    return subscriberId_ == other.subscriberId_ && uid_ == other.uid_;
  }

  struct Hash {
    std::size_t operator()(const SubscriptionIdentifier& id) const {
      if (id.uid_ != 0) {
        // if uid is set it is guaranteed to be unique, so only hash that
        return id.uid_;
      } else {
        return folly::hash::hash_combine(id.subscriberId_);
      }
    }
  };

 private:
  SubscriberId subscriberId_;
  uint64_t uid_;
};

} // namespace facebook::fboss::fsdb
