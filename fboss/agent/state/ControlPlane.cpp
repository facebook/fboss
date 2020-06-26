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

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
constexpr auto kQueues = "queues";
constexpr auto kRxReasonToQueue = "rxReasonToQueue";
constexpr auto rxReasonToQueueOrderedList = "rxReasonToQueueOrderedList";
constexpr auto kRxReason = "rxReason";
constexpr auto kQueueId = "queueId";
constexpr auto kQosPolicy = "defaultQosPolicy";
} // namespace

namespace facebook::fboss {

folly::dynamic ControlPlaneFields::toFollyDynamic() const {
  folly::dynamic controlPlane = folly::dynamic::object;
  controlPlane[kQueues] = folly::dynamic::array;
  for (const auto& queue : queues) {
    controlPlane[kQueues].push_back(queue->toFollyDynamic());
  }

  controlPlane[rxReasonToQueueOrderedList] = folly::dynamic::array;
  // TODO(pgardideh): temporarily write the object version. remove when
  // migration is complete
  controlPlane[kRxReasonToQueue] = folly::dynamic::object;
  for (const auto& entry : rxReasonToQueue) {
    auto reason = apache::thrift::util::enumName(entry.rxReason);
    CHECK(reason != nullptr);
    controlPlane[rxReasonToQueueOrderedList].push_back(
        folly::dynamic::object(kRxReason, reason)(kQueueId, entry.queueId));

    controlPlane[kRxReasonToQueue][reason] = entry.queueId;
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
    for (const auto& queueJson : json[kQueues]) {
      auto queue = PortQueue::fromFollyDynamic(queueJson);
      controlPlane.queues.push_back(queue);
    }
  }
  if (json.find(rxReasonToQueueOrderedList) != json.items().end()) {
    for (const auto& reasonToQueueEntry : json[rxReasonToQueueOrderedList]) {
      CHECK(
          reasonToQueueEntry.find(kRxReason) !=
          reasonToQueueEntry.items().end());
      CHECK(
          reasonToQueueEntry.find(kQueueId) !=
          reasonToQueueEntry.items().end());
      cfg::PacketRxReason reason;
      const int found = apache::thrift::util::tryParseEnum(
          reasonToQueueEntry.at(kRxReason).asString(), &reason);
      CHECK(found);
      cfg::PacketRxReasonToQueue reasonToQueue;
      reasonToQueue.rxReason_ref() = reason;
      reasonToQueue.queueId_ref() = reasonToQueueEntry.at(kQueueId).asInt();
      controlPlane.rxReasonToQueue.push_back(reasonToQueue);
    }
  } else if (json.find(kRxReasonToQueue) != json.items().end()) {
    // TODO(pgardideh): the map version of reason to queue is deprecated. Remove
    // this read when it is safe to do so.
    for (const auto& reasonToQueueJson : json[kRxReasonToQueue].items()) {
      cfg::PacketRxReason reason;
      const int found = apache::thrift::util::tryParseEnum(
          reasonToQueueJson.first.asString(), &reason);
      CHECK(found);
      cfg::PacketRxReasonToQueue reasonToQueue;
      reasonToQueue.rxReason_ref() = reason;
      reasonToQueue.queueId_ref() = reasonToQueueJson.second.asInt();
      controlPlane.rxReasonToQueue.push_back(reasonToQueue);
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

cfg::PacketRxReasonToQueue ControlPlane::makeRxReasonToQueueEntry(
    cfg::PacketRxReason reason,
    uint16_t queueId) {
  cfg::PacketRxReasonToQueue reasonToQueue;
  reasonToQueue.rxReason = reason;
  reasonToQueue.queueId = queueId;
  return reasonToQueue;
}

bool ControlPlane::operator==(const ControlPlane& controlPlane) const {
  // TODO(joseph5wu) Will add QueueConfig struct in the future diff.
  auto compareQueues = [&](const QueueConfig& queues1,
                           const QueueConfig& queues2) -> bool {
    if (queues1.size() != queues2.size()) {
      return false;
    }
    for (int i = 0; i < queues1.size(); i++) {
      if (*(queues1.at(i)) != *(queues2.at(i))) {
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

} // namespace facebook::fboss
