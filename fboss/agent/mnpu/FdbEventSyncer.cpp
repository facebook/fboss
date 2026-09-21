/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/FdbEventSyncer.h"
#if FOLLY_HAS_COROUTINES
#include <folly/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

FdbEventSyncer::FdbEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb,
    std::optional<std::string> multiSwitchStatsPrefix)
    : ThriftSinkClient<multiswitch::FdbEvent, FdbEventQueueType>::
          ThriftSinkClient(
              "FdbEventThriftSyncer",
              serverPort,
              switchId,
              FdbEventSyncer::initFdbEventSink,
              std::make_shared<folly::ScopedEventBaseThread>(
                  "FdbEventSyncerThread"),
#if FOLLY_HAS_COROUTINES
              eventQueue_,
#endif
              connRetryEvb,
              multiSwitchStatsPrefix) {
}

ThriftSinkClient<multiswitch::FdbEvent, FdbEventQueueType>::
    EventNotifierSinkClient
    FdbEventSyncer::initFdbEventSink(
        SwitchID switchId,
        apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
#if FOLLY_HAS_COROUTINES
  return folly::coro::blockingWait(client->co_notifyFdbEvent(switchId));
#else
  return apache::thrift::ClientSink<multiswitch::FdbEvent, bool>();
#endif
}

} // namespace facebook::fboss
