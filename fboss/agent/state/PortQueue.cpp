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

namespace {
constexpr auto kId = "id";
constexpr auto kStreamType = "streamType";
constexpr auto kPriority = "priority";
constexpr auto kWeight = "weight";
}

namespace facebook { namespace fboss {

folly::dynamic PortQueueFields::toFollyDynamic() const {
  folly::dynamic queue = folly::dynamic::object;
  if (weight) {
    queue[kWeight] = weight.value();
  }
  if (priority) {
    queue[kPriority] = priority.value();
  }
  queue[kId] = id;
  queue[kStreamType] =
    cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second;
  return queue;
}

PortQueueFields PortQueueFields::fromFollyDynamic(
    const folly::dynamic& queueJson) {
  PortQueueFields queue(static_cast<uint8_t>(queueJson[kId].asInt()));

  cfg::StreamType streamType = cfg::_StreamType_NAMES_TO_VALUES.find(
      queueJson[kStreamType].asString().c_str())->second;
  queue.streamType = streamType;
  if (queueJson.find(kPriority) != queueJson.items().end()) {
    queue.priority = queueJson[kPriority].asInt();
  }
  if (queueJson.find(kWeight) != queueJson.items().end()) {
    queue.weight = queueJson[kWeight].asInt();
  }
  return queue;
}

PortQueue::PortQueue(uint8_t id) : NodeBaseT(id) {
}

template class NodeBaseT<PortQueue, PortQueueFields>;

}} // facebook::fboss
