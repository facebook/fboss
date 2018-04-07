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
  using PacketCounter = cfg::PacketCounterMatchAction;

  MatchAction() {}

  MatchAction(const MatchAction& action)
    : sendToQueue_(action.sendToQueue_),
      packetCounter_(action.packetCounter_) {}

  folly::Optional<SendToQueue> getSendToQueue() const {
    return sendToQueue_;
  }

  void setSendToQueue(const SendToQueue& sendToQueue) {
    sendToQueue_ = sendToQueue;
  }

  folly::Optional<PacketCounter> getPacketCounter() const {
    return packetCounter_;
  }

  void setPacketCounter(const PacketCounter& packetCounter) {
    packetCounter_ = packetCounter;
  }

  bool operator==(const MatchAction& action) const {
    return sendToQueue_ == action.sendToQueue_ &&
      packetCounter_ == action.packetCounter_;
  }

  MatchAction& operator=(const MatchAction& action) {
    sendToQueue_ = action.sendToQueue_;
    packetCounter_ = action.packetCounter_;
    return *this;
  }

  folly::dynamic toFollyDynamic() const;
  static MatchAction fromFollyDynamic(const folly::dynamic& actionJson);

 private:
  folly::Optional<SendToQueue> sendToQueue_{folly::none};
  folly::Optional<PacketCounter> packetCounter_{folly::none};
};

}} // facebook::fboss
