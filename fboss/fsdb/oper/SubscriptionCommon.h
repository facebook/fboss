// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

#include <folly/hash/Hash.h>

namespace facebook::fboss::fsdb {

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
