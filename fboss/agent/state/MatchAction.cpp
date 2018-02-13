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
  return matchAction;
}
}} // facebook::fboss
