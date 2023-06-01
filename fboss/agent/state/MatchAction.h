/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/FBString.h>
#include <folly/dynamic.h>
#include <optional>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook::fboss {

class MatchAction {
 public:
  /*
   * In config, we will get rid of `sendToCPU` and use `CPUTrafficPolicyConfig`
   * to decide whether we need to send to cpu queue or regular port queue.
   * In agent code, we still need to keep this `sendToCPU` logic for acl, thus,
   * SendToQueue.second is the old `sendToCPU` value.
   */
  using SendToQueue = std::pair<cfg::QueueMatchAction, bool>;
  using SetDscp = cfg::SetDscpMatchAction;
  using MacsecFlow = cfg::MacsecFlowAction;
  using NextHopSet = RouteNextHopEntry::NextHopSet;
  using RedirectToNextHopAction =
      std::pair<cfg::RedirectToNextHopAction, NextHopSet>;

  MatchAction() {}

  MatchAction(const MatchAction& action)
      : sendToQueue_(action.sendToQueue_),
        trafficCounter_(action.trafficCounter_),
        setDscp_(action.setDscp_),
        ingressMirror_(action.ingressMirror_),
        egressMirror_(action.egressMirror_),
        toCpuAction_(action.toCpuAction_),
        macsecFlow_(action.macsecFlow_),
        redirectToNextHop_(action.redirectToNextHop_) {}

  std::optional<SendToQueue> getSendToQueue() const {
    return sendToQueue_;
  }

  void setSendToQueue(const SendToQueue& sendToQueue) {
    sendToQueue_ = sendToQueue;
  }

  bool isSendToCpuQueue() const {
    return sendToQueue_ && sendToQueue_.value().second;
  }

  std::optional<cfg::TrafficCounter> getTrafficCounter() const {
    return trafficCounter_;
  }

  void setTrafficCounter(const cfg::TrafficCounter& trafficCounter) {
    trafficCounter_ = trafficCounter;
  }

  std::optional<SetDscp> getSetDscp() const {
    return setDscp_;
  }

  void setSetDscp(const SetDscp& setDscp) {
    setDscp_ = setDscp;
  }

  std::optional<MacsecFlow> getMacsecFlow() const {
    return macsecFlow_;
  }

  void setMacsecFlow(const MacsecFlow& macsecFlow) {
    macsecFlow_ = macsecFlow;
  }

  std::optional<std::string> getIngressMirror() const {
    return ingressMirror_;
  }

  void setIngressMirror(const std::string& mirror) {
    ingressMirror_ = mirror;
  }

  void unsetIngressMirror() {
    ingressMirror_.reset();
  }

  std::optional<std::string> getEgressMirror() const {
    return egressMirror_;
  }

  void unsetEgressMirror() {
    egressMirror_.reset();
  }

  void setEgressMirror(const std::string& mirror) {
    egressMirror_ = mirror;
  }

  std::optional<cfg::ToCpuAction> getToCpuAction() const {
    return toCpuAction_;
  }

  void setToCpuAction(const cfg::ToCpuAction& toCpuAction) {
    toCpuAction_ = toCpuAction;
  }

  const std::optional<RedirectToNextHopAction>& getRedirectToNextHop() const {
    return redirectToNextHop_;
  }

  void setRedirectToNextHop(const RedirectToNextHopAction& redirectToNextHop) {
    redirectToNextHop_ = redirectToNextHop;
  }

  bool operator==(const MatchAction& action) const {
    return std::tie(
               sendToQueue_,
               ingressMirror_,
               egressMirror_,
               trafficCounter_,
               setDscp_,
               toCpuAction_,
               macsecFlow_,
               redirectToNextHop_) ==
        std::tie(
               action.sendToQueue_,
               action.ingressMirror_,
               action.egressMirror_,
               action.trafficCounter_,
               action.setDscp_,
               action.toCpuAction_,
               action.macsecFlow_,
               action.redirectToNextHop_);
  }

  MatchAction& operator=(const MatchAction& action) {
    std::tie(
        sendToQueue_,
        ingressMirror_,
        egressMirror_,
        trafficCounter_,
        setDscp_,
        toCpuAction_,
        macsecFlow_,
        redirectToNextHop_) =
        std::tie(
            action.sendToQueue_,
            action.ingressMirror_,
            action.egressMirror_,
            action.trafficCounter_,
            action.setDscp_,
            action.toCpuAction_,
            action.macsecFlow_,
            action.redirectToNextHop_);
    return *this;
  }

  state::MatchAction toThrift() const;
  static MatchAction fromThrift(state::MatchAction const& ma);

 private:
  std::optional<SendToQueue> sendToQueue_{std::nullopt};
  std::optional<cfg::TrafficCounter> trafficCounter_{std::nullopt};
  std::optional<SetDscp> setDscp_{std::nullopt};
  std::optional<std::string> ingressMirror_{std::nullopt};
  std::optional<std::string> egressMirror_{std::nullopt};
  std::optional<cfg::ToCpuAction> toCpuAction_{std::nullopt};
  std::optional<MacsecFlow> macsecFlow_{std::nullopt};
  std::optional<RedirectToNextHopAction> redirectToNextHop_{std::nullopt};
};

} // namespace facebook::fboss
