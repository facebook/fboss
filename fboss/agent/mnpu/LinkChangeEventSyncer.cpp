/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/LinkChangeEventSyncer.h"
#include "fboss/agent/HwSwitch.h"
#if FOLLY_HAS_COROUTINES
#include <folly/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

LinkChangeEventSyncer::LinkChangeEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb,
    HwSwitch* hw,
    std::optional<std::string> multiSwitchStatsPrefix)
    : ThriftSinkClient<multiswitch::LinkChangeEvent, LinkChangeEventQueueType>::
          ThriftSinkClient(
              "LinkChangeEventThriftSyncer",
              serverPort,
              switchId,
              LinkChangeEventSyncer::initLinkChangeEventSink,
              std::make_shared<folly::ScopedEventBaseThread>(
                  "LinkChangeEventSyncerThread"),
#if FOLLY_HAS_COROUTINES
              eventQueue_,
#endif
              connRetryEvb,
              multiSwitchStatsPrefix),
      hw_(hw) {
}

LinkChangeEventSyncer::EventSink LinkChangeEventSyncer::initLinkChangeEventSink(
    SwitchID switchId,
    apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
#if FOLLY_HAS_COROUTINES
  return folly::coro::blockingWait(client->co_notifyLinkChangeEvent(switchId));
#else
  return apache::thrift::ClientSink<multiswitch::LinkChangeEvent, bool>();
#endif
}

void LinkChangeEventSyncer::connected() {
  hw_->syncLinkStates();
  hw_->syncLinkActiveStates();
  hw_->syncLinkConnectivity();
}

} // namespace facebook::fboss
