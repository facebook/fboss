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

folly::dynamic ControlPlaneFields::toFollyDynamicLegacy() const {
  folly::dynamic controlPlane = folly::dynamic::object;
  controlPlane[kQueues] = folly::dynamic::array;
  for (auto queue : queues()) {
    controlPlane[kQueues].push_back(queue->toFollyDynamic());
  }

  controlPlane[kRxReasonToQueueOrderedList] = folly::dynamic::array;
  // TODO(pgardideh): temporarily write the object version. remove when
  // migration is complete
  controlPlane[kRxReasonToQueue] = folly::dynamic::object;
  for (auto entry : rxReasonToQueue()) {
    auto reason = apache::thrift::util::enumName(*entry.rxReason());
    CHECK(reason != nullptr);
    controlPlane[kRxReasonToQueueOrderedList].push_back(
        folly::dynamic::object(kRxReason, reason)(kQueueId, *entry.queueId()));

    controlPlane[kRxReasonToQueue][reason] = *entry.queueId();
  }

  if (auto policy = qosPolicy()) {
    controlPlane[kQosPolicy] = *policy;
  }

  return controlPlane;
}

ControlPlaneFields ControlPlaneFields::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  ControlPlaneFields controlPlane = ControlPlaneFields();
  QueueConfig queues;
  if (json.find(kQueues) != json.items().end()) {
    for (const auto& queueJson : json[kQueues]) {
      auto queue = PortQueue::fromFollyDynamic(queueJson);
      queues.push_back(queue);
    }
  }
  controlPlane.setQueues(std::move(queues));
  RxReasonToQueue rxReasonToQueue;
  if (json.find(kRxReasonToQueueOrderedList) != json.items().end()) {
    for (const auto& reasonToQueueEntry : json[kRxReasonToQueueOrderedList]) {
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
      reasonToQueue.rxReason() = reason;
      reasonToQueue.queueId() = reasonToQueueEntry.at(kQueueId).asInt();
      rxReasonToQueue.push_back(reasonToQueue);
    }
    controlPlane.setRxReasonToQueue(std::move(rxReasonToQueue));
  } else if (json.find(kRxReasonToQueue) != json.items().end()) {
    // TODO(pgardideh): the map version of reason to queue is deprecated. Remove
    // this read when it is safe to do so.
    for (const auto& reasonToQueueJson : json[kRxReasonToQueue].items()) {
      cfg::PacketRxReason reason;
      const int found = apache::thrift::util::tryParseEnum(
          reasonToQueueJson.first.asString(), &reason);
      CHECK(found);
      cfg::PacketRxReasonToQueue reasonToQueue;
      reasonToQueue.rxReason() = reason;
      reasonToQueue.queueId() = reasonToQueueJson.second.asInt();
      rxReasonToQueue.push_back(reasonToQueue);
    }
    controlPlane.setRxReasonToQueue(std::move(rxReasonToQueue));
  }
  if (json.find(kQosPolicy) != json.items().end()) {
    controlPlane.setQosPolicy(json[kQosPolicy].asString());
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
  *reasonToQueue.rxReason() = reason;
  *reasonToQueue.queueId() = queueId;
  return reasonToQueue;
}

state::ControlPlaneFields ControlPlaneFields::toThrift() const {
  return data();
}

ControlPlaneFields ControlPlaneFields::fromThrift(
    state::ControlPlaneFields const& thriftControlPlaneFields) {
  return ControlPlaneFields(thriftControlPlaneFields);
}

folly::dynamic ControlPlaneFields::migrateToThrifty(folly::dynamic const& dyn) {
  folly::dynamic newDynamic = folly::dynamic::object;
  if (dyn.find(kQosPolicy) != dyn.items().end()) {
    newDynamic[kQosPolicy] = dyn[kQosPolicy];
  }
  newDynamic[kQueues] = dyn[kQueues];

  folly::dynamic rxReasonToQueueDynamic = folly::dynamic::array;
  for (auto entry : dyn[kRxReasonToQueueOrderedList]) {
    folly::dynamic entryDynamic = folly::dynamic::object;
    auto rxReason = apache::thrift::util::enumValueOrThrow<cfg::PacketRxReason>(
        entry[kRxReason].asString());
    auto queueId = entry[kQueueId].asInt();
    entryDynamic[kRxReason] = static_cast<int>(rxReason);
    entryDynamic[kQueueId] = queueId;
    rxReasonToQueueDynamic.push_back(entryDynamic);
  }
  newDynamic[kRxReasonToQueue] = rxReasonToQueueDynamic;
  return newDynamic;
}

void ControlPlaneFields::migrateFromThrifty(folly::dynamic& dyn) {
  auto rxReasonToQueue = dyn[kRxReasonToQueue];
  folly::dynamic rxReasonToQueueOrderedListLegacy = folly::dynamic::array;
  folly::dynamic rxReasonToQueueLegacy = folly::dynamic::object;
  for (auto entry : rxReasonToQueue) {
    folly::dynamic rxReasonToQueueDynamicLegacy = folly::dynamic::object;

    auto reasonName = apache::thrift::util::enumName<cfg::PacketRxReason>(
        static_cast<cfg::PacketRxReason>(entry[kRxReason].asInt()));

    rxReasonToQueueDynamicLegacy[kRxReason] = reasonName;
    rxReasonToQueueDynamicLegacy[kQueueId] = entry[kQueueId].asInt();
    rxReasonToQueueOrderedListLegacy.push_back(rxReasonToQueueDynamicLegacy);
    rxReasonToQueueLegacy[reasonName] = entry[kQueueId].asInt();
  }
  dyn.erase(kRxReasonToQueue);
  dyn[kRxReasonToQueueOrderedList] = rxReasonToQueueOrderedListLegacy;
  dyn[kRxReasonToQueue] = rxReasonToQueueLegacy;
}

template class ThriftStructNode<ControlPlane, state::ControlPlaneFields>;

} // namespace facebook::fboss
