/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Thrifty.h"
#include "folly/dynamic.h"
#include "folly/json.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
constexpr auto kQueues = "queues";
constexpr auto kRxReasonToQueue = "rxReasonToQueue";
constexpr auto kRxReasonToQueueOrderedList = "rxReasonToQueueOrderedList";
constexpr auto kRxReason = "rxReason";
constexpr auto kQueueId = "queueId";
constexpr auto kQosPolicy = "defaultQosPolicy";
} // namespace

namespace facebook::fboss {

ControlPlane* ControlPlane::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newControlPlane = clone();
  auto* ptr = newControlPlane.get();
  (*state)->resetControlPlane(std::move(newControlPlane));
  return ptr;
}

cfg::PacketRxReasonToQueue ControlPlane::makeRxReasonToQueueEntry(
    cfg::PacketRxReason reason,
    uint16_t queueId) {
  cfg::PacketRxReasonToQueue reasonToQueue;
  *reasonToQueue.rxReason() = reason;
  *reasonToQueue.queueId() = queueId;
  return reasonToQueue;
}

template class ThriftStructNode<ControlPlane, state::ControlPlaneFields>;

} // namespace facebook::fboss
