// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>

#include "fboss/agent/types.h"

namespace facebook::fboss {

// Mints NextHopSetIDs for EcmpResourceManager's merged (override) groups.
class OverrideSetIdManager {
 public:
  static bool isOverrideSetID(NextHopSetID id) {
    return nextHopIdType(static_cast<int64_t>(id)) ==
        NextHopIdType::OVERRIDE_SET;
  }

 private:
  static constexpr int64_t kStart =
      nextHopIdTypeStart(NextHopIdType::OVERRIDE_SET);
};

} // namespace facebook::fboss
