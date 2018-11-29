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

#include <folly/dynamic.h>
#include <folly/FBString.h>
#include <folly/Optional.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {

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

  folly::Optional<SendToQueue> getSendToQueue() const {
    return sendToQueue_;
  }

  void setSendToQueue(const SendToQueue& sendToQueue) {
    sendToQueue_ = sendToQueue;
  }

  folly::Optional<cfg::TrafficCounter> getTrafficCounter() const {
    return trafficCounter_;
  }

  void setTrafficCounter(const cfg::TrafficCounter& trafficCounter) {
    trafficCounter_ = trafficCounter;
  }

  folly::Optional<SetDscp> getSetDscp() const {
    return setDscp_;
  }

  void setSetDscp(const SetDscp& setDscp) {
    setDscp_ = setDscp;
  }

  folly::Optional<std::string> getIngressMirror() const {
    return ingressMirror_;
  }

  void setIngressMirror(const std::string& mirror) {
    ingressMirror_.assign(mirror);
  }

  void unsetIngressMirror() {
    ingressMirror_.clear();
  }

  folly::Optional<std::string> getEgressMirror() const {
    return egressMirror_;
  }

  void unsetEgressMirror() {
    egressMirror_.clear();
  }

  void setEgressMirror(const std::string& mirror) {
    egressMirror_.assign(mirror);
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
  folly::Optional<SendToQueue> sendToQueue_{folly::none};
  folly::Optional<cfg::TrafficCounter> trafficCounter_{folly::none};
  folly::Optional<SetDscp> setDscp_{folly::none};
  folly::Optional<std::string> ingressMirror_{folly::none};
  folly::Optional<std::string> egressMirror_{folly::none};
};

}} // facebook::fboss
