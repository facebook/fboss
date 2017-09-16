/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/MatchToActionEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include <folly/Conv.h>

namespace {
constexpr auto kMatcher = "matcher";
constexpr auto kAction = "action";
constexpr auto kQueueMatchAction = "queueMatchAction";
constexpr auto kQueueId = "queueId";
}

namespace facebook { namespace fboss {

folly::dynamic MatchToActionEntryFields::toFollyDynamic() const {
  folly::dynamic mtae = folly::dynamic::object;
  mtae[kMatcher] = matcher->toFollyDynamic();
  mtae[kAction] = folly::dynamic::object;
  if (action.__isset.sendToQueue) {
    mtae[kAction][kQueueMatchAction] = folly::dynamic::object;
    mtae[kAction][kQueueMatchAction][kQueueId] = action.sendToQueue.queueId;
  }
  return mtae;
}

MatchToActionEntryFields MatchToActionEntryFields::fromFollyDynamic(
    const folly::dynamic& mtaeJson) {
  MatchToActionEntryFields mtae;
  mtae.matcher = AclEntry::fromFollyDynamic(mtaeJson[kMatcher]);
  cfg::MatchAction action;
  if (mtaeJson[kAction].find(kQueueMatchAction) != mtaeJson.items().end()) {
    action.sendToQueue = cfg::QueueMatchAction();
    action.__isset.sendToQueue = true;
    action.sendToQueue.queueId =
      mtaeJson[kAction][kQueueMatchAction][kQueueId].asInt();
  }
  mtae.action = action;
  return mtae;
}

MatchToActionEntry::MatchToActionEntry()
  : NodeBaseT() {
}

template class NodeBaseT<MatchToActionEntry, MatchToActionEntryFields>;

}} // facebook::fboss
