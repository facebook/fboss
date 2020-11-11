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
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/Port.h"

DEFINE_int32(mka_service_port, 5990, "mka_service thrift port");
DEFINE_int32(fboss_mka_port, 6990, "fboss agent mka service packet I/O port");
DEFINE_double(
    mka_reconnect_timer,
    1000,
    "re-connect to mka_service timer in milliseconds");

namespace facebook::fboss {
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
  auto port = folly::to<std::string>(packet->getSrcPort());
  if (!stream_->isPortRegistered(port)) {
    VLOG(3) << "Port '" << port << "' not registered by mka service";
    return;
  }
  folly::IOBuf* buf = packet->buf();
  TPacket pktToSend;
  pktToSend.buf_ref() = buf->moveToFbString().toStdString();
  pktToSend.timestamp_ref() = time(nullptr);
  pktToSend.l2Port_ref() = std::move(port);
  size_t len = pktToSend.buf_ref()->size();
  if (len != stream_->send(std::move(pktToSend))) {
    LOG(ERROR) << "Failed to send MkPdu packet received on Port:'"
               << packet->getSrcPort() << "' to mka_service";
  }
}

void MKAServiceManager::recvPacket(TPacket&& packet) {
  if (!packet.l2Port_ref()->size() || !packet.buf_ref()->size()) {
    LOG(ERROR) << "Invalid packet received from MKA Service";
    return;
  }
  auto txPkt = swSwitch_->allocatePacket(packet.buf_ref()->size());
  folly::io::RWPrivateCursor cursor(txPkt->buf());
  cursor.push(
      reinterpret_cast<const uint8_t*>(packet.buf_ref()->data()),
      packet.buf_ref()->size());
  try {
    swSwitch_->sendPacketOutOfPortAsync(
        std::move(txPkt), PortID(folly::to<uint16_t>(*packet.l2Port_ref())));
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Failed to MkPdu Packet to the switch, port:"
               << *packet.l2Port_ref() << " ex: " << ex.what();
  }
} // namespace facebook::fboss

} // namespace facebook::fboss
