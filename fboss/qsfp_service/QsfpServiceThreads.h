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

#include <memory>
#include <unordered_map>

#include "fboss/agent/types.h"
#include "fboss/qsfp_service/SlotThreadHelper.h"

namespace facebook::fboss {

class PlatformMapping;

/*
 * QsfpServiceThreads encapsulates the threading infrastructure used by
 * qsfp_service for managing transceiver state machine updates. This struct
 * will be extended in the future to include additional platform-derived data.
 */
struct QsfpServiceThreads {
  using TcvrToThreadIdMap = std::unordered_map<TransceiverID, int>;
  using XphyToThreadIdMap = std::unordered_map<XphyId, int>;
  using PortToThreadIdMap = std::unordered_map<PortID, int>;
  using ThreadIdToThreadMap = std::unordered_map<int, SlotThreadHelper>;

  TcvrToThreadIdMap tcvrToThreadId;
  XphyToThreadIdMap xphyToThreadId;
  PortToThreadIdMap portToThreadId;
  ThreadIdToThreadMap threadIdToThread;
};

/*
 * Factory function to create and initialize QsfpServiceThreads based on
 * platform mapping. Creates one SlotThreadHelper per transceiver ID found
 * in the platform mapping.
 */
std::shared_ptr<QsfpServiceThreads> createQsfpServiceThreads(
    const std::shared_ptr<const PlatformMapping>& platformMapping);

/*
 * Get the EventBase pointer for a given TransceiverID.
 * Returns nullptr if the TransceiverID is not found in the thread map.
 */
folly::EventBase* getEventBaseForTcvr(
    const std::shared_ptr<QsfpServiceThreads>& threads,
    TransceiverID tcvrId);

/*
 * Get the EventBase pointer for a given PortID.
 * Returns nullptr if the PortID is not found in the thread map.
 */
folly::EventBase* getEventBaseForPort(
    const std::shared_ptr<QsfpServiceThreads>& threads,
    PortID portId);

} // namespace facebook::fboss
