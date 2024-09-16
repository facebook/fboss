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
#include <folly/coro/BlockingWait.h>
#endif

DEFINE_int32(tx_pkt_stream_buffer_size, 1000, "TxPktEventStream buffer size");

namespace facebook::fboss {

apache::thrift::RpcOptions TxPktEventSyncer::getRpcOptions() {
  apache::thrift::RpcOptions options;
  options.setChunkBufferSize(FLAGS_tx_pkt_stream_buffer_size);
  return options;
}

TxPktEventSyncer::TxPktEventSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    folly::EventBase* connRetryEvb,
    HwSwitch* hw,
    std::optional<std::string> multiSwitchStatsPrefix)
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
          connRetryEvb,
          multiSwitchStatsPrefix) {
}

#if FOLLY_HAS_COROUTINES
ThriftStreamClient<multiswitch::TxPacket>::EventNotifierStreamClient
TxPktEventSyncer::initTxPktEventStream(
    SwitchID switchId,
    apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client) {
  auto options = TxPktEventSyncer::getRpcOptions();
  return folly::coro::blockingWait(client->co_getTxPackets(options, switchId))
      .toAsyncGenerator();
}
#endif

fb303::TimeseriesWrapper& TxPktEventSyncer::getBadPacketCounter() {
  static fb303::TimeseriesWrapper txBadPktReceived{
      "tx_bad_pkt_received", fb303::SUM, fb303::RATE};
  return txBadPktReceived;
}

void TxPktEventSyncer::TxPacketEventHandler(
    multiswitch::TxPacket& txPkt,
    HwSwitch* hw) {
  if (hw->getRunState() == SwitchRunState::EXITING) {
    XLOG(DBG4) << "TxPktEventSyncer: hwswitch exiting, dropping packet";
    return;
  }
  auto len = (*txPkt.data())->computeChainDataLength();
  auto pkt = hw->allocatePacket(len);
  folly::io::Cursor inCursor(txPkt.data()->get());
  folly::io::RWPrivateCursor outCursor(pkt->buf());
  outCursor.pushAtMost(inCursor, len);

  if (*txPkt.length() != len) {
    XLOG(ERR) << "Tx packet length mismatch for switch " << *hw->getSwitchId();
    getBadPacketCounter().add(1);
    return;
  }

  if (txPkt.port().has_value()) {
    PortID portId(*txPkt.port());
    hw->sendPacketOutOfPortAsync(
        std::move(pkt), portId, txPkt.queue().to_optional());
  } else {
    hw->sendPacketSwitchedAsync(std::move(pkt));
  }
}

} // namespace facebook::fboss
