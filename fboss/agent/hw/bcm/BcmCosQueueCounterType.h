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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

/*
 * Since bcm stats type is not Bcm supported, define our enums instead.
 */
enum class BcmCosQueueStatType {
  DROPPED_PACKETS,
  DROPPED_BYTES,
  OUT_PACKETS,
  OUT_BYTES,
  OBM_LOSSY_HIGH_PRI_DROPPED_PACKETS,
  OBM_LOSSY_HIGH_PRI_DROPPED_BYTES,
  OBM_LOSSY_LOW_PRI_DROPPED_PACKETS,
  OBM_LOSSY_LOW_PRI_DROPPED_BYTES,
  OBM_HIGH_WATERMARK,
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

  cfg::CounterType getCounterType() const {
    switch (statType) {
      case BcmCosQueueStatType::DROPPED_PACKETS:
      case BcmCosQueueStatType::OUT_PACKETS:
      case BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_PACKETS:
      case BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_PACKETS:
        return cfg::CounterType::PACKETS;
      case BcmCosQueueStatType::DROPPED_BYTES:
      case BcmCosQueueStatType::OUT_BYTES:
      case BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_BYTES:
      case BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_BYTES:
        return cfg::CounterType::BYTES;
      case BcmCosQueueStatType::OBM_HIGH_WATERMARK:
        throw FbossError("Invalid stat type");
    }
    throw FbossError("Invalid stat type");
  }
};

} // namespace facebook::fboss
