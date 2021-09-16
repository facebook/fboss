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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/Thrifty.h"

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
constexpr auto kToCpuAction = "cpuAction";

// These names match the thrift definition in switch_config.thrift
constexpr auto kThriftSendToQueue = "sendToQueue";
constexpr auto kThriftSendToCPU = "sendToCPU";
constexpr auto kThriftAction = "action";
constexpr auto kThriftTrafficCounter = "trafficCounter";
constexpr auto kThriftSetDscp = "setDscp";
constexpr auto kThriftToCpuAction = "toCpuAction";
} // namespace

namespace facebook::fboss {

state::MatchAction MatchAction::toThrift() const {
  auto matchAction = state::MatchAction();
  if (sendToQueue_.has_value()) {
    auto toQueue = state::SendToQueue();
    toQueue.action_ref() = sendToQueue_->first;
    toQueue.sendToCPU_ref() = sendToQueue_->second;
    matchAction.sendToQueue_ref() = toQueue;
  }
  matchAction.trafficCounter_ref().from_optional(trafficCounter_);
  matchAction.setDscp_ref().from_optional(setDscp_);
  matchAction.ingressMirror_ref().from_optional(ingressMirror_);
  matchAction.egressMirror_ref().from_optional(egressMirror_);
  matchAction.toCpuAction_ref().from_optional(toCpuAction_);
  matchAction.macsecFlow_ref().from_optional(macsecFlow_);
  return matchAction;
}

MatchAction MatchAction::fromThrift(state::MatchAction const& ma) {
  auto matchAction = MatchAction();
  if (auto sendToQueue = ma.sendToQueue_ref()) {
    auto toQueue = SendToQueue();
    toQueue.first = sendToQueue->get_action();
    toQueue.second = sendToQueue->get_sendToCPU();
    matchAction.sendToQueue_ = toQueue;
  }
  matchAction.trafficCounter_ = ma.trafficCounter_ref().to_optional();
  matchAction.setDscp_ = ma.setDscp_ref().to_optional();
  matchAction.ingressMirror_ = ma.ingressMirror_ref().to_optional();
  matchAction.egressMirror_ = ma.egressMirror_ref().to_optional();
  matchAction.toCpuAction_ = ma.toCpuAction_ref().to_optional();
  matchAction.macsecFlow_ = ma.macsecFlow_ref().to_optional();
  return matchAction;
}

// TODO: remove all migration along with old ser/des after next disruptive push
folly::dynamic MatchAction::migrateToThrifty(const folly::dynamic& dyn) {
  folly::dynamic newDyn = dyn;

  if (auto it = newDyn.find(kQueueMatchAction); it != newDyn.items().end()) {
    const auto action = it->second;
    newDyn[kThriftSendToQueue] = folly::dynamic::object;
    newDyn[kThriftSendToQueue][kThriftSendToCPU] = action[kSendToCPU].asBool();
    newDyn[kThriftSendToQueue][kThriftAction] = action;
  }
  ThriftyUtils::renameField(newDyn, kCounter, kThriftTrafficCounter);
  ThriftyUtils::renameField(newDyn, kSetDscpMatchAction, kThriftSetDscp);
  ThriftyUtils::renameField(newDyn, kToCpuAction, kThriftToCpuAction);

  return newDyn;
}

void MatchAction::migrateFromThrifty(folly::dynamic& dyn) {
  if (auto it = dyn.find(kThriftSendToQueue); it != dyn.items().end()) {
    const auto action = it->second[kThriftAction];
    const auto sendToCPU = it->second[kThriftSendToCPU];
    dyn[kQueueMatchAction] = action;
    dyn[kQueueMatchAction][kSendToCPU] = sendToCPU;
  }
  ThriftyUtils::renameField(dyn, kThriftTrafficCounter, kCounter);
  ThriftyUtils::renameField(dyn, kThriftSetDscp, kSetDscpMatchAction);
  ThriftyUtils::renameField(dyn, kThriftToCpuAction, kToCpuAction);
}

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
  if (toCpuAction_) {
    matchAction[kToCpuAction] = static_cast<int>(toCpuAction_.value());
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
  if (actionJson.find(kToCpuAction) != actionJson.items().end()) {
    matchAction.setToCpuAction(
        static_cast<cfg::ToCpuAction>(actionJson[kToCpuAction].asInt()));
  }
  return matchAction;
}
} // namespace facebook::fboss
