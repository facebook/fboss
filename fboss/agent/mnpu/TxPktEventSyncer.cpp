/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/TxPktEventSyncer.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/BlockingWait.h>
#endif

namespace facebook::fboss {

TxPktEventSyncer::TxPktEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb,
    HwSwitch* hw)
    : ThriftStreamClient<multiswitch::TxPacket>::ThriftStreamClient(
          "TxPktEventThriftSyncer",
          serverPort,
          switchId,
#if FOLLY_HAS_COROUTINES
          TxPktEventSyncer::initTxPktEventStream,
#endif
          TxPktEventSyncer::TxPacketEventHandler,
          hw,
          std::make_shared<folly::ScopedEventBaseThread>(
              "TxPktEventSyncerThread"),
          connRetryEvb) {
}

#if FOLLY_HAS_COROUTINES
ThriftStreamClient<multiswitch::TxPacket>::EventNotifierStreamClient
TxPktEventSyncer::initTxPktEventStream(
    SwitchID switchId,
    apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
  return folly::coro::blockingWait(client->co_getTxPackets(switchId))
      .toAsyncGenerator();
}
#endif

void TxPktEventSyncer::TxPacketEventHandler(
    multiswitch::TxPacket& txPkt,
    HwSwitch* hw) {
  auto len = (*txPkt.data())->computeChainDataLength();
  auto pkt = hw->allocatePacket(len);
  folly::io::RWPrivateCursor cursor(pkt->buf());
  cursor.push((*txPkt.data())->data(), len);
  if (txPkt.port().has_value()) {
    PortID portId(*txPkt.port());
    std::optional<uint8_t> queue;
    if (txPkt.queue().has_value()) {
      queue = *txPkt.queue();
    }
    hw->sendPacketOutOfPortAsync(std::move(pkt), portId, queue);
  } else {
    hw->sendPacketSwitchedAsync(std::move(pkt));
  }
}

} // namespace facebook::fboss
