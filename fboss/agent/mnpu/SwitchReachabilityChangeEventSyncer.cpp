/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/SwitchReachabilityChangeEventSyncer.h"
#include "fboss/agent/HwSwitch.h"
#if FOLLY_HAS_COROUTINES
#include <folly/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

SwitchReachabilityChangeEventSyncer::SwitchReachabilityChangeEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb,
    HwSwitch* hw,
    std::optional<std::string> multiSwitchStatsPrefix)
    : ThriftSinkClient<
          multiswitch::SwitchReachabilityChangeEvent,
          SwitchReachabilityChangeEventQueueType>::
          ThriftSinkClient(
              "SwitchReachabilityChangeEventThriftSyncer",
              serverPort,
              switchId,
              SwitchReachabilityChangeEventSyncer::
                  initSwitchReachabilityChangeEventSink,
              std::make_shared<folly::ScopedEventBaseThread>(
                  "SwitchReachabilityChangeEventSyncerThread"),
#if FOLLY_HAS_COROUTINES
              eventQueue_,
#endif
              connRetryEvb,
              std::move(multiSwitchStatsPrefix)),
      hw_(hw) {
}

SwitchReachabilityChangeEventSyncer::EventSink
SwitchReachabilityChangeEventSyncer::initSwitchReachabilityChangeEventSink(
    SwitchID switchId,
    apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
#if FOLLY_HAS_COROUTINES
  return folly::coro::blockingWait(
      client->co_notifySwitchReachabilityChangeEvent(switchId));
#else
  return apache::thrift::
      ClientSink<multiswitch::SwitchReachabilityChangeEvent, bool>();
#endif
}

void SwitchReachabilityChangeEventSyncer::connected() {
  hw_->syncSwitchReachability();
}

} // namespace facebook::fboss
