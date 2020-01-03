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

  MatchAction() {}

  MatchAction(const MatchAction& action)
      : sendToQueue_(action.sendToQueue_),
        trafficCounter_(action.trafficCounter_),
        setDscp_(action.setDscp_),
        ingressMirror_(action.ingressMirror_),
        egressMirror_(action.egressMirror_) {}

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

  bool operator==(const MatchAction& action) const {
    return sendToQueue_ == action.sendToQueue_ &&
        ingressMirror_ == action.ingressMirror_ &&
        egressMirror_ == action.egressMirror_ &&
        trafficCounter_ == action.trafficCounter_ &&
        setDscp_ == action.setDscp_;
  }

  MatchAction& operator=(const MatchAction& action) {
    sendToQueue_ = action.sendToQueue_;
    trafficCounter_ = action.trafficCounter_;
    setDscp_ = action.setDscp_;
    ingressMirror_ = action.ingressMirror_;
    egressMirror_ = action.egressMirror_;
    return *this;
  }

  folly::dynamic toFollyDynamic() const;
  static MatchAction fromFollyDynamic(const folly::dynamic& actionJson);

 private:
  std::optional<SendToQueue> sendToQueue_{std::nullopt};
  std::optional<cfg::TrafficCounter> trafficCounter_{std::nullopt};
  std::optional<SetDscp> setDscp_{std::nullopt};
  std::optional<std::string> ingressMirror_{std::nullopt};
  std::optional<std::string> egressMirror_{std::nullopt};
};

} // namespace facebook::fboss
