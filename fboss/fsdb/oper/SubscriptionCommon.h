// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

namespace facebook::fboss::fsdb {

// SubscriptionIdentifier: helper to facilitate referencing a
// subscription by a unique identifier or alternate key.
class SubscriptionIdentifier {
 public:
  explicit SubscriptionIdentifier(SubscriberId subscriberId, uint64_t uid = 0)
      : subscriberId_(std::move(subscriberId)), uid_(uid) {}
  const SubscriberId& subscriberId() const {
    return subscriberId_;
  }
  uint64_t uid() const {
    return uid_;
  }

 private:
  SubscriberId subscriberId_;
  uint64_t uid_;
};

} // namespace facebook::fboss::fsdb
