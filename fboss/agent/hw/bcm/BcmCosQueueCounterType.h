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

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

/*
 * Since bcm stats type is not Bcm supported, define our enums instead.
 */
enum class BcmCosQueueStatType {
  DROPPED_PACKETS,
  DROPPED_BYTES,
  OUT_PACKETS,
  OUT_BYTES
};

enum class BcmCosQueueCounterScope {
  QUEUES, // only collect each single queue stat
  AGGREGATED, // only collect aggregated queue stat
  QUEUES_AND_AGGREGATED // collect both single queue and aggregated stats
};

struct BcmCosQueueCounterType {
  cfg::StreamType streamType;
  BcmCosQueueStatType statType;
  BcmCosQueueCounterScope scope;
  folly::StringPiece name;

  bool operator<(const BcmCosQueueCounterType& t) const {
    return std::tie(streamType, statType, scope, name) <
        std::tie(t.streamType, t.statType, t.scope, t.name);
  }

  bool isScopeQueues() const {
    return scope == BcmCosQueueCounterScope::QUEUES ||
        scope == BcmCosQueueCounterScope::QUEUES_AND_AGGREGATED;
  }
  bool isScopeAggregated() const {
    return scope == BcmCosQueueCounterScope::AGGREGATED ||
        scope == BcmCosQueueCounterScope::QUEUES_AND_AGGREGATED;
  }
};

} // namespace facebook::fboss
