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
constexpr auto kPacketCounterMatchAction = "packetCounterMatchAction";
constexpr auto kCounterName = "counterName";
constexpr auto kSetDscpMatchAction = "setDscpMatchAction";
constexpr auto kDscpValue = "dscpValue";
constexpr auto kIngressMirror = "ingressMirror";
constexpr auto kEgressMirror = "engressMirror";
}

namespace facebook { namespace fboss {

folly::dynamic MatchAction::toFollyDynamic() const {
  folly::dynamic matchAction = folly::dynamic::object;
  if (sendToQueue_) {
    matchAction[kQueueMatchAction] = folly::dynamic::object;
    matchAction[kQueueMatchAction][kQueueId] =
      sendToQueue_.value().first.queueId;
    matchAction[kQueueMatchAction][kSendToCPU] = sendToQueue_.value().second;
  }
  if (packetCounter_) {
    matchAction[kPacketCounterMatchAction] = folly::dynamic::object;
    matchAction[kPacketCounterMatchAction][kCounterName] =
      packetCounter_.value().counterName;
  }
  if (setDscp_) {
    matchAction[kSetDscpMatchAction] = folly::dynamic::object;
    matchAction[kSetDscpMatchAction][kDscpValue] = setDscp_.value().dscpValue;
  }
  if (ingressMirror_) {
    matchAction[kIngressMirror] = ingressMirror_.value();
  }
  if (egressMirror_) {
    matchAction[kEgressMirror] = egressMirror_.value();
  }
  return matchAction;
}

MatchAction MatchAction::fromFollyDynamic(
    const folly::dynamic& actionJson) {
  MatchAction matchAction = MatchAction();
  if (actionJson.find(kQueueMatchAction) != actionJson.items().end()) {
    cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
    queueAction.queueId = actionJson[kQueueMatchAction][kQueueId].asInt();
    bool sendToCPU = actionJson[kQueueMatchAction][kSendToCPU].asBool();
    matchAction.setSendToQueue(std::make_pair(queueAction, sendToCPU));
  }
  if (actionJson.find(kPacketCounterMatchAction) != actionJson.items().end()) {
    auto packetCounterAction = cfg::PacketCounterMatchAction();
    packetCounterAction.counterName =
        actionJson[kPacketCounterMatchAction][kCounterName].asString();
    matchAction.setPacketCounter(packetCounterAction);
  }
  if (actionJson.find(kSetDscpMatchAction) != actionJson.items().end()) {
    auto setDscpMatchAction= cfg::SetDscpMatchAction();
    setDscpMatchAction.dscpValue=
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
}} // facebook::fboss
