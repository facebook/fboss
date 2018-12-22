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
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"

namespace {
constexpr auto kQueues = "queues";
constexpr auto kRxReasonToQueue = "rxReasonToQueue";
constexpr auto kQosPolicy = "defaultQosPolicy";
}

namespace facebook { namespace fboss {

folly::dynamic ControlPlaneFields::toFollyDynamic() const {
  folly::dynamic controlPlane = folly::dynamic::object;
  controlPlane[kQueues] = folly::dynamic::array;
  for (const auto& queue: queues) {
    controlPlane[kQueues].push_back(queue->toFollyDynamic());
  }
  controlPlane[kRxReasonToQueue] = folly::dynamic::object;

  for (const auto& reasonToQueue: rxReasonToQueue) {
    auto reason = cfg::_PacketRxReason_VALUES_TO_NAMES.find(
      reasonToQueue.first);
    CHECK(reason != cfg::_PacketRxReason_VALUES_TO_NAMES.end());
    controlPlane[kRxReasonToQueue][reason->second] = reasonToQueue.second;
  }

  if (qosPolicy) {
    controlPlane[kQosPolicy] = *qosPolicy;
  }

  return controlPlane;
}

ControlPlaneFields ControlPlaneFields::fromFollyDynamic(
    const folly::dynamic& json) {
  ControlPlaneFields controlPlane = ControlPlaneFields();
  if (json.find(kQueues) != json.items().end()) {
    for (const auto& queueJson: json[kQueues]) {
      auto queue = PortQueue::fromFollyDynamic(queueJson);
      controlPlane.queues.push_back(queue);
    }
  }
  if (json.find(kRxReasonToQueue) != json.items().end()) {
    for (const auto& reasonToQueueJson: json[kRxReasonToQueue].items()) {
      auto reason = cfg::_PacketRxReason_NAMES_TO_VALUES.find(
        reasonToQueueJson.first.asString().c_str());
      CHECK(reason != cfg::_PacketRxReason_NAMES_TO_VALUES.end());
      controlPlane.rxReasonToQueue.emplace(reason->second,
                                  reasonToQueueJson.second.asInt());
    }
  }
  if (json.find(kQosPolicy) != json.items().end()) {
    controlPlane.qosPolicy = json[kQosPolicy].asString();
  }
  return controlPlane;
}

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

bool ControlPlane::operator==(const ControlPlane& controlPlane) const {
  // TODO(joseph5wu) Will add QueueConfig struct in the future diff.
  auto compareQueues = [&] (const QueueConfig& queues1,
                            const QueueConfig& queues2)
                       -> bool {
    if (queues1.size() != queues2.size()) {
      return false;
    }
    for (int i = 0; i < queues1.size(); i++) {
      if(*(queues1.at(i)) != *(queues2.at(i))) {
        return false;
      }
    }
    return true;
  };

  return compareQueues(getFields()->queues, controlPlane.getQueues()) &&
      getFields()->rxReasonToQueue == controlPlane.getRxReasonToQueue() &&
      getFields()->qosPolicy == controlPlane.getQosPolicy();
}

template class NodeBaseT<ControlPlane, ControlPlaneFields>;
}} // facebook::fboss
