/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/MKAServiceManager.h"
#include <folly/io/Cursor.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include "fboss/agent/PortStats.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/Port.h"

DEFINE_double(
    mka_reconnect_timer,
    1000,
    "re-connect to mka_service timer in milliseconds");

namespace facebook::fboss {

#define CHECK_STATS(stats, fn) \
  do {                         \
    if (stats) {               \
      fn;                      \
    }                          \
  } while (0)

MKAServiceManager::MKAServiceManager(SwSwitch* sw) : swSwitch_(sw) {
  clientThread_ = std::make_unique<folly::ScopedEventBaseThread>(
      "mka_service_client_thread");
  timerThread_ = std::make_unique<folly::ScopedEventBaseThread>(
      "mka_service_timer_thread");
  stream_ = std::make_shared<BidirectionalPacketStream>(
      "fboss_mka_service_conn",
      clientThread_->getEventBase(),
      timerThread_->getEventBase(),
      FLAGS_mka_reconnect_timer);
  stream_->setPacketAcceptor(this);
  serverThread_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
      stream_, "::1", FLAGS_fboss_mka_port);
  stream_->connectClient(FLAGS_mka_service_port);
}

MKAServiceManager::~MKAServiceManager() {
  stream_->stopClient();
  timerThread_->getEventBase()->terminateLoopSoon();
  clientThread_->getEventBase()->terminateLoopSoon();
  stream_.reset();
}

uint16_t MKAServiceManager::getServerPort() const {
  return (serverThread_ ? serverThread_->getPort() : 0);
}

void MKAServiceManager::handlePacket(std::unique_ptr<RxPacket> packet) {
  if (!packet) {
    return;
  }
  PortID port = packet->getSrcPort();
  auto portStr = folly::to<std::string>(port);
  PortStats* stats = swSwitch_->portStats(port);

  if (!stream_->isPortRegistered(portStr)) {
    CHECK_STATS(stats, stats->MkPduPortNotRegistered());
    XLOG(DBG3) << "Port '" << portStr << "' not registered by mka service";
    return;
  }
  folly::IOBuf* buf = packet->buf();
  TPacket pktToSend;
  pktToSend.buf_ref() = buf->moveToFbString().toStdString();
  pktToSend.timestamp_ref() = time(nullptr);
  pktToSend.l2Port_ref() = std::move(portStr);
  size_t len = pktToSend.buf_ref()->size();
  if (len != stream_->send(std::move(pktToSend))) {
    CHECK_STATS(stats, stats->MKAServiceSendFailue());
    XLOG(ERR) << "Failed to send MkPdu packet received on Port:'"
              << packet->getSrcPort() << "' to mka_service";
    return;
  }
  stats->MKAServiceSendSuccess();
}

void MKAServiceManager::recvPacket(TPacket&& packet) {
  if (!packet.l2Port_ref()->size() || !packet.buf_ref()->size()) {
    XLOG(ERR) << "Invalid packet received from MKA Service";
    return;
  }
  PortID port;
  try {
    port = PortID(folly::to<uint16_t>(*packet.l2Port_ref()));
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Invalid MkPdu Port:  " << ex.what();
    return;
  }
  auto txPkt = swSwitch_->allocatePacket(packet.buf_ref()->size());
  folly::io::RWPrivateCursor cursor(txPkt->buf());
  cursor.push(
      reinterpret_cast<const uint8_t*>(packet.buf_ref()->data()),
      packet.buf_ref()->size());

  PortStats* stats = swSwitch_->portStats(port);
  CHECK_STATS(stats, stats->MKAServiceRecvSuccess());
  try {
    swSwitch_->sendPacketOutOfPortAsync(std::move(txPkt), port);
    CHECK_STATS(stats, stats->MkPduSendPkt());
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to MkPdu Packet to the switch, port:"
              << *packet.l2Port_ref() << " ex: " << ex.what();
    CHECK_STATS(stats, stats->MkPduSendFailure());
  }
}

} // namespace facebook::fboss
