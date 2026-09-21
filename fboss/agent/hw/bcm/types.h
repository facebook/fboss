/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/types.h"

#include <unordered_map>

FBOSS_STRONG_TYPE(int, BcmAclStatHandle);
FBOSS_STRONG_TYPE(int, BcmQosPolicyHandle);

namespace facebook::fboss {

using BcmAclEntryHandle = int;
using BcmMirrorHandle = int;
using BcmTeFlowEntryHandle = int;

using BcmTrafficCounterStats = std::unordered_map<cfg::CounterType, uint64_t>;
using BcmEgressQueueTrafficCounterStats = std::unordered_map<
    cfg::StreamType,
    std::unordered_map<int, BcmTrafficCounterStats>>;

enum class BcmMmuState {
  UNKNOWN,
  MMU_LOSSLESS,
  MMU_LOSSY,
  MMU_LOSSY_AND_LOSSLESS
};
} // namespace facebook::fboss
