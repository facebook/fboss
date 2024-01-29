/*
+ *  Copyright (c) 2004-present, Facebook, Inc.
+ *  All rights reserved.
+ *
+ *  This source code is licensed under the BSD-style license found in the
+ *  LICENSE file in the root directory of this source tree. An additional grant
+ *  of patent rights can be found in the PATENTS file in the same directory.
+ *
+ */
#include "fboss/agent/mnpu/LinkActiveEventSyncer.h"
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

LinkActiveEventSyncer::LinkActiveEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb)
    : ThriftSinkClient<multiswitch::LinkActiveEvent>::ThriftSinkClient(
          "LinkActiveEventThriftSyncer",
          serverPort,
          switchId,
          LinkActiveEventSyncer::initLinkActiveEventSink,
          std::make_shared<folly::ScopedEventBaseThread>(
              "LinkActiveEventSyncerThread"),
          connRetryEvb) {}

LinkActiveEventSyncer::EventSink LinkActiveEventSyncer::initLinkActiveEventSink(
    SwitchID switchId,
    apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
#if FOLLY_HAS_COROUTINES
  return folly::coro::blockingWait(client->co_notifyLinkActiveEvent(switchId));
#else
  return apache::thrift::ClientSink<multiswitch::LinkActiveEvent, bool>();
#endif
}

} // namespace facebook::fboss
