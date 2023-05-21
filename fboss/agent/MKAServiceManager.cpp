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
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"

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

std::string MKAServiceManager::getPortName(PortID portId) const {
  return swSwitch_->getState()->getPorts()->getPort(portId)->getName();
}

void MKAServiceManager::handlePacket(std::unique_ptr<RxPacket> packet) {
  if (!packet) {
    return;
  }
  if (pausePduForTest_) {
    XLOG(DBG5)
        << "MKAServiceManager not handling this packet for testing purpose";
    return;
  }
  PortID port = packet->getSrcPort();
  auto l2Port = getPortName(port);
  PortStats* stats = swSwitch_->portStats(port);

  stats->MkPduInterval();

  if (!stream_->isPortRegistered(l2Port)) {
    l2Port = folly::to<std::string>(packet->getSrcPort());
    if (!stream_->isPortRegistered(l2Port)) {
      CHECK_STATS(stats, stats->MkPduPortNotRegistered());
      XLOG(DBG3) << "Port '" << l2Port << "' not registered by mka service";
      return;
    }
  }
  folly::IOBuf* buf = packet->buf();
  TPacket pktToSend;
  pktToSend.buf() = buf->moveToFbString().toStdString();
  pktToSend.timestamp() = time(nullptr);
  pktToSend.l2Port() = std::move(l2Port);
  size_t len = pktToSend.buf()->size();
  if (len != stream_->send(std::move(pktToSend))) {
    CHECK_STATS(stats, stats->MKAServiceSendFailue());
    XLOG(ERR) << "Failed to send MkPdu packet received on Port:'"
              << packet->getSrcPort() << "' to mka_service";
    return;
  }
  stats->MKAServiceSendSuccess();
}

void MKAServiceManager::recvPacket(TPacket&& packet) {
  if (!packet.l2Port()->size() || !packet.buf()->size()) {
    XLOG(ERR) << "Invalid packet received from MKA Service";
    return;
  }
  PortID port;
  VlanID vlan;
  try {
    try {
      port = PortID(folly::to<uint16_t>(*packet.l2Port()));
    } catch (const std::exception& e) {
      port = swSwitch_->getState()
                 ->getMultiSwitchPorts()
                 ->getPort(*packet.l2Port())
                 ->getID();
    }
    vlan = swSwitch_->getState()
               ->getPorts()
               ->getPort(port)
               ->getVlans()
               .begin()
               ->first;
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Invalid MkPdu Port:  " << ex.what();
    return;
  }

  auto txPkt =
      swSwitch_->allocatePacket(packet.buf()->size() + 4 /*802.1q header*/);
  auto mkPduBuffer = folly::IOBuf::wrapBuffer(
      reinterpret_cast<const uint8_t*>(packet.buf()->data()),
      packet.buf()->size());
  folly::io::Cursor mkPduCursor(mkPduBuffer.get());
  auto dstMac = PktUtil::readMac(&mkPduCursor);
  auto srcMac = PktUtil::readMac(&mkPduCursor);
  auto mkEtherType = mkPduCursor.readBE<uint16_t>();

  folly::io::RWPrivateCursor cursor(txPkt->buf());
  TxPacket::writeEthHeader(&cursor, dstMac, srcMac, vlan, mkEtherType);

  cursor.push(
      mkPduCursor,
      packet.buf()->size() -
          14 /*dmac + smac + ethertype written as part of eth header above*/);
  PortStats* stats = swSwitch_->portStats(port);
  CHECK_STATS(stats, stats->MKAServiceRecvSuccess());
  try {
    swSwitch_->sendNetworkControlPacketAsync(
        std::move(txPkt), PortDescriptor(port));
    CHECK_STATS(stats, stats->MkPduSendPkt());
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to MkPdu Packet to the switch, port:"
              << *packet.l2Port() << " ex: " << ex.what();
    CHECK_STATS(stats, stats->MkPduSendFailure());
  }
}

} // namespace facebook::fboss
