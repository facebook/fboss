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
constexpr auto kNexthops = "nexthops";
constexpr auto kNexthopIp = "ip";
constexpr auto kNexthopIntfID = "intfID";
constexpr auto kRedirectNextHops = "redirectNextHops";
constexpr auto kResolvedNexthops = "resolvedNexthops";
constexpr auto kRedirectToNextHop = "redirectToNextHop";

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
    toQueue.action() = sendToQueue_->first;
    toQueue.sendToCPU() = sendToQueue_->second;
    matchAction.sendToQueue() = toQueue;
  }
  matchAction.trafficCounter().from_optional(trafficCounter_);
  matchAction.setDscp().from_optional(setDscp_);
  matchAction.ingressMirror().from_optional(ingressMirror_);
  matchAction.egressMirror().from_optional(egressMirror_);
  matchAction.toCpuAction().from_optional(toCpuAction_);
  matchAction.macsecFlow().from_optional(macsecFlow_);
  if (redirectToNextHop_.has_value()) {
    auto redirectToNextHop = state::RedirectToNextHopAction();
    redirectToNextHop.action() = redirectToNextHop_->first;
    for (const auto& nh : redirectToNextHop_->second) {
      redirectToNextHop.resolvedNexthops()->push_back(nh.toThrift());
    }
    matchAction.redirectToNextHop() = redirectToNextHop;
  }
  return matchAction;
}

MatchAction MatchAction::fromThrift(state::MatchAction const& ma) {
  auto matchAction = MatchAction();
  if (auto sendToQueue = ma.sendToQueue()) {
    auto toQueue = SendToQueue();
    toQueue.first = sendToQueue->get_action();
    toQueue.second = sendToQueue->get_sendToCPU();
    matchAction.sendToQueue_ = toQueue;
  }
  matchAction.trafficCounter_ = ma.trafficCounter().to_optional();
  matchAction.setDscp_ = ma.setDscp().to_optional();
  matchAction.ingressMirror_ = ma.ingressMirror().to_optional();
  matchAction.egressMirror_ = ma.egressMirror().to_optional();
  matchAction.toCpuAction_ = ma.toCpuAction().to_optional();
  matchAction.macsecFlow_ = ma.macsecFlow().to_optional();
  if (auto redirectToNextHop = ma.redirectToNextHop()) {
    auto redirectAction = RedirectToNextHopAction();
    redirectAction.first = redirectToNextHop->get_action();
    for (const auto& nh : redirectToNextHop->get_resolvedNexthops()) {
      redirectAction.second.insert(
          util::fromThrift(nh, true /* allowV6NonLinkLocal */));
    }
    matchAction.redirectToNextHop_ = redirectAction;
  }
  return matchAction;
}

folly::dynamic MatchAction::toFollyDynamic() const {
  folly::dynamic matchAction = folly::dynamic::object;
  if (sendToQueue_) {
    matchAction[kQueueMatchAction] = folly::dynamic::object;
    matchAction[kQueueMatchAction][kQueueId] =
        *sendToQueue_.value().first.queueId();
    matchAction[kQueueMatchAction][kSendToCPU] = sendToQueue_.value().second;
  }
  if (trafficCounter_) {
    matchAction[kCounter] = folly::dynamic::object;
    matchAction[kCounter][kCounterName] = *trafficCounter_.value().name();
    matchAction[kCounter][kCounterTypes] = folly::dynamic::array;
    for (const auto& type : *trafficCounter_.value().types()) {
      matchAction[kCounter][kCounterTypes].push_back(static_cast<int>(type));
    }
  }
  if (setDscp_) {
    matchAction[kSetDscpMatchAction] = folly::dynamic::object;
    matchAction[kSetDscpMatchAction][kDscpValue] =
        *setDscp_.value().dscpValue();
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
  if (redirectToNextHop_) {
    matchAction[kRedirectToNextHop] = folly::dynamic::object;
    matchAction[kRedirectToNextHop][kThriftAction] = folly::dynamic::object;
    matchAction[kRedirectToNextHop][kThriftAction][kNexthops] =
        folly::dynamic::array;
    for (const auto& nexthop : *redirectToNextHop_.value().first.nexthops()) {
      matchAction[kRedirectToNextHop][kThriftAction][kNexthops].push_back(
          nexthop);
    }
    matchAction[kRedirectToNextHop][kThriftAction][kRedirectNextHops] =
        folly::dynamic::array;
    for (const auto& nexthop :
         *redirectToNextHop_.value().first.redirectNextHops()) {
      folly::dynamic nhop = folly::dynamic::object;
      nhop[kNexthopIp] = *nexthop.ip();
      if (nexthop.intfID().has_value()) {
        nhop[kNexthopIntfID] = nexthop.intfID().value();
      }
      matchAction[kRedirectToNextHop][kThriftAction][kRedirectNextHops]
          .push_back(nhop);
    }
    folly::dynamic nhops = folly::dynamic::array;
    for (const auto& nh : redirectToNextHop_.value().second) {
      std::string nhJson;
      apache::thrift::SimpleJSONSerializer::serialize(nh.toThrift(), &nhJson);
      nhops.push_back(folly::parseJson(nhJson));
    }
    matchAction[kRedirectToNextHop][kResolvedNexthops] = nhops;
  }
  return matchAction;
}

MatchAction MatchAction::fromFollyDynamic(const folly::dynamic& actionJson) {
  MatchAction matchAction = MatchAction();
  if (actionJson.find(kQueueMatchAction) != actionJson.items().end()) {
    cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
    *queueAction.queueId() = actionJson[kQueueMatchAction][kQueueId].asInt();
    bool sendToCPU = actionJson[kQueueMatchAction][kSendToCPU].asBool();
    matchAction.setSendToQueue(std::make_pair(queueAction, sendToCPU));
  }
  if (actionJson.find(kCounter) != actionJson.items().end()) {
    auto counter = cfg::TrafficCounter();
    *counter.name() = actionJson[kCounter][kCounterName].asString();
    counter.types()->clear();
    for (const auto& type : actionJson[kCounter][kCounterTypes]) {
      counter.types()->push_back(static_cast<cfg::CounterType>(type.asInt()));
    }
    matchAction.setTrafficCounter(counter);
  }
  // TODO(adrs): get rid of this (backward compatibility)
  constexpr auto kPacketCounterMatchAction = "packetCounterMatchAction";
  constexpr auto kCounterName = "counterName";
  if (actionJson.find(kPacketCounterMatchAction) != actionJson.items().end()) {
    auto counter = cfg::TrafficCounter();
    *counter.name() =
        actionJson[kPacketCounterMatchAction][kCounterName].asString();
    *counter.types() = {cfg::CounterType::PACKETS};
    matchAction.setTrafficCounter(counter);
  }

  if (actionJson.find(kSetDscpMatchAction) != actionJson.items().end()) {
    auto setDscpMatchAction = cfg::SetDscpMatchAction();
    *setDscpMatchAction.dscpValue() =
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
  if (actionJson.find(kRedirectToNextHop) != actionJson.items().end()) {
    auto redirectToNextHop = cfg::RedirectToNextHopAction();
    redirectToNextHop.redirectNextHops()->clear();
    for (const auto& nh :
         actionJson[kRedirectToNextHop][kThriftAction][kRedirectNextHops]) {
      cfg::RedirectNextHop nhop;
      nhop.ip_ref() = nh[kNexthopIp].asString();
      if (nh.find(kNexthopIntfID) != nh.items().end()) {
        nhop.intfID_ref() = nh[kNexthopIntfID].asInt();
      }
      redirectToNextHop.redirectNextHops()->push_back(nhop);
    }
    redirectToNextHop.nexthops()->clear();
    for (const auto& nh :
         actionJson[kRedirectToNextHop][kThriftAction][kNexthops]) {
      redirectToNextHop.nexthops()->push_back(nh.asString());
    }
    auto resolvedNhops = NextHopSet();
    for (const auto& nhValue :
         actionJson[kRedirectToNextHop][kResolvedNexthops]) {
      NextHopThrift nh;
      apache::thrift::SimpleJSONSerializer::deserialize<NextHopThrift>(
          folly::toJson(nhValue), nh);
      resolvedNhops.insert(
          util::fromThrift(nh, true /* allowV6NonLinkLocal */));
    }
    matchAction.setRedirectToNextHop(
        std::make_pair(redirectToNextHop, resolvedNhops));
  }
  return matchAction;
}
} // namespace facebook::fboss
