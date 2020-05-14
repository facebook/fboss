/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/MatchAction.h"
#include <folly/Conv.h>

namespace {
constexpr auto kQueueMatchAction = "queueMatchAction";
constexpr auto kQueueId = "queueId";
constexpr auto kSendToCPU = "sendToCPU";
constexpr auto kSetDscpMatchAction = "setDscpMatchAction";
constexpr auto kDscpValue = "dscpValue";
constexpr auto kIngressMirror = "ingressMirror";
constexpr auto kEgressMirror = "engressMirror";
constexpr auto kCounter = "counter";
constexpr auto kCounterName = "name";
constexpr auto kCounterTypes = "types";
} // namespace

namespace facebook::fboss {

folly::dynamic MatchAction::toFollyDynamic() const {
  folly::dynamic matchAction = folly::dynamic::object;
  if (sendToQueue_) {
    matchAction[kQueueMatchAction] = folly::dynamic::object;
    matchAction[kQueueMatchAction][kQueueId] =
        *sendToQueue_.value().first.queueId_ref();
    matchAction[kQueueMatchAction][kSendToCPU] = sendToQueue_.value().second;
  }
  if (trafficCounter_) {
    matchAction[kCounter] = folly::dynamic::object;
    matchAction[kCounter][kCounterName] = *trafficCounter_.value().name_ref();
    matchAction[kCounter][kCounterTypes] = folly::dynamic::array;
    for (const auto& type : *trafficCounter_.value().types_ref()) {
      matchAction[kCounter][kCounterTypes].push_back(static_cast<int>(type));
    }
  }
  if (setDscp_) {
    matchAction[kSetDscpMatchAction] = folly::dynamic::object;
    matchAction[kSetDscpMatchAction][kDscpValue] =
        *setDscp_.value().dscpValue_ref();
  }
  if (ingressMirror_) {
    matchAction[kIngressMirror] = ingressMirror_.value();
  }
  if (egressMirror_) {
    matchAction[kEgressMirror] = egressMirror_.value();
  }
  return matchAction;
}

MatchAction MatchAction::fromFollyDynamic(const folly::dynamic& actionJson) {
  MatchAction matchAction = MatchAction();
  if (actionJson.find(kQueueMatchAction) != actionJson.items().end()) {
    cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
    *queueAction.queueId_ref() =
        actionJson[kQueueMatchAction][kQueueId].asInt();
    bool sendToCPU = actionJson[kQueueMatchAction][kSendToCPU].asBool();
    matchAction.setSendToQueue(std::make_pair(queueAction, sendToCPU));
  }
  if (actionJson.find(kCounter) != actionJson.items().end()) {
    auto counter = cfg::TrafficCounter();
    *counter.name_ref() = actionJson[kCounter][kCounterName].asString();
    counter.types_ref()->clear();
    for (const auto& type : actionJson[kCounter][kCounterTypes]) {
      counter.types_ref()->push_back(
          static_cast<cfg::CounterType>(type.asInt()));
    }
    matchAction.setTrafficCounter(counter);
  }
  // TODO(adrs): get rid of this (backward compatibility)
  constexpr auto kPacketCounterMatchAction = "packetCounterMatchAction";
  constexpr auto kCounterName = "counterName";
  if (actionJson.find(kPacketCounterMatchAction) != actionJson.items().end()) {
    auto counter = cfg::TrafficCounter();
    *counter.name_ref() =
        actionJson[kPacketCounterMatchAction][kCounterName].asString();
    *counter.types_ref() = {cfg::CounterType::PACKETS};
    matchAction.setTrafficCounter(counter);
  }

  if (actionJson.find(kSetDscpMatchAction) != actionJson.items().end()) {
    auto setDscpMatchAction = cfg::SetDscpMatchAction();
    *setDscpMatchAction.dscpValue_ref() =
        actionJson[kSetDscpMatchAction][kDscpValue].asInt();
    matchAction.setSetDscp(setDscpMatchAction);
  }
  if (actionJson.find(kIngressMirror) != actionJson.items().end()) {
    matchAction.setIngressMirror(actionJson[kIngressMirror].asString());
  }
  if (actionJson.find(kEgressMirror) != actionJson.items().end()) {
    matchAction.setEgressMirror(actionJson[kEgressMirror].asString());
  }
  return matchAction;
}

} // namespace facebook::fboss
