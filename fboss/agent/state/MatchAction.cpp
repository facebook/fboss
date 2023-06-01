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

} // namespace facebook::fboss
