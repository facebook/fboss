/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include <folly/Conv.h>

namespace facebook { namespace fboss {

state::PortQueueFields PortQueueFields::toThrift() const {
  state::PortQueueFields queue;
  queue.weight = weight;
  queue.reserved = reservedBytes;
  if (scalingFactor) {
    queue.scalingFactor =
        cfg::_MMUScalingFactor_VALUES_TO_NAMES.at(*scalingFactor);
  }
  queue.id = id;
  queue.scheduling = cfg::_QueueScheduling_VALUES_TO_NAMES.at(scheduling);
  queue.streamType = cfg::_StreamType_VALUES_TO_NAMES.at(streamType);
  queue.aqm = aqm;
  return queue;
}

// static, public
PortQueueFields PortQueueFields::fromThrift(
    state::PortQueueFields const& queueThrift) {
  PortQueueFields queue(static_cast<uint8_t>(queueThrift.id));

  auto const itrStreamType = cfg::_StreamType_NAMES_TO_VALUES.find(
      queueThrift.streamType.c_str());
  CHECK(itrStreamType != cfg::_StreamType_NAMES_TO_VALUES.end());
  queue.streamType = itrStreamType->second;

  auto const itrSched = cfg::_QueueScheduling_NAMES_TO_VALUES.find(
      queueThrift.scheduling.c_str());
  CHECK(itrSched != cfg::_QueueScheduling_NAMES_TO_VALUES.end());
  queue.scheduling = itrSched->second;

  queue.reservedBytes = queueThrift.reserved;
  queue.weight = queueThrift.weight;

  if (queueThrift.scalingFactor) {
    auto itrScalingFactor = cfg::_MMUScalingFactor_NAMES_TO_VALUES.find(
        queueThrift.scalingFactor->c_str());
    CHECK(itrScalingFactor != cfg::_MMUScalingFactor_NAMES_TO_VALUES.end());
    queue.scalingFactor = itrScalingFactor->second;
  }

  queue.aqm = queueThrift.aqm;

  return queue;
}

PortQueue::PortQueue(uint8_t id) : ThriftyBaseT(id) {
}

template class NodeBaseT<PortQueue, PortQueueFields>;

}} // facebook::fboss
