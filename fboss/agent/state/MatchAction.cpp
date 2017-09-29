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
#include "fboss/agent/state/StateUtils.h"
#include <folly/Conv.h>

namespace {
constexpr auto kQueueMatchAction = "queueMatchAction";
constexpr auto kQueueId = "queueId";
constexpr auto kSendToCpu = "sendToCpu";
}

namespace facebook { namespace fboss {

const folly::dynamic MatchAction::toFollyDynamic(
    const cfg::MatchAction& action) {
  folly::dynamic matchAction = folly::dynamic::object;
  if (action.__isset.sendToQueue) {
    matchAction[kQueueMatchAction] = folly::dynamic::object;
    matchAction[kQueueMatchAction][kQueueId] =
      action.sendToQueue.queueId;
  }
  matchAction[kSendToCpu] = action.sendToCPU;
  return matchAction;
}

const cfg::MatchAction MatchAction::fromFollyDynamic(
    const folly::dynamic& json) {
  cfg::MatchAction matchAction;
  if (json.find(kQueueMatchAction) != json.items().end()) {
    matchAction.sendToQueue = cfg::QueueMatchAction();
    matchAction.__isset.sendToQueue = true;
    matchAction.sendToQueue.queueId =
      json[kQueueMatchAction][kQueueId].asInt();
  }
  if (json.find(kSendToCpu) != json.items().end()) {
    // TODO: This if statement can be removed once we've warmbooted into
    // the new config format
    matchAction.sendToCPU = json[kSendToCpu].asBool();
  }
  return matchAction;
}
}} // facebook::fboss
