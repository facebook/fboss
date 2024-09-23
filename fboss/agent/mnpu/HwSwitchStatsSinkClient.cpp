/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/HwSwitchStatsSinkClient.h"
#if FOLLY_HAS_COROUTINES
#include <folly/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

HwSwitchStatsSinkClient::HwSwitchStatsSinkClient(
    uint16_t serverPort,
    SwitchID switchId,
    uint16_t switchIndex,
    folly::EventBase* connRetryEvb,
    std::optional<std::string> multiSwitchStatsPrefix)
    : ThriftSinkClient<multiswitch::HwSwitchStats, StatsEventQueueType>::
          ThriftSinkClient(
              "HwSwitchStatsSinkClient",
              serverPort,
              switchId,
              [this, switchIdx = switchIndex](
                  SwitchID switchId,
                  apache::thrift::Client<multiswitch::MultiSwitchCtrl>*
                      client) {
                return initHwSwitchStatsSinkClient(switchId, switchIdx, client);
              },
              std::make_shared<folly::ScopedEventBaseThread>(
                  "HwSwitchStatsSinkClientThread"),
#if FOLLY_HAS_COROUTINES
              eventQueue_,
#endif
              connRetryEvb,
              multiSwitchStatsPrefix) {
}

ThriftSinkClient<multiswitch::HwSwitchStats, StatsEventQueueType>::
    EventNotifierSinkClient
    HwSwitchStatsSinkClient::initHwSwitchStatsSinkClient(
        SwitchID /*switchId*/,
        uint16_t switchIndex,
        apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
#if FOLLY_HAS_COROUTINES
  return folly::coro::blockingWait(client->co_syncHwStats(switchIndex));
#else
  return apache::thrift::ClientSink<multiswitch::HwSwitchStats, bool>();
#endif
}

} // namespace facebook::fboss
