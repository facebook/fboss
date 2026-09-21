// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/PacketStreamHandler.h"
#include "fboss/agent/PortDescriptorTemplate.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/Port.h"

#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

PacketStreamHandler::PacketStreamHandler(
    SwSwitch* sw,
    const std::string& serviceStream,
    const std::string& statsName)
    : PacketStreamService(serviceStream, false),
      sw_(sw),
      fromExternalAgentReceivedCount_(
          "from" + statsName + ".packets_received",
          fb303::SUM,
          fb303::RATE),
      fromExternalAgentDroppedCount_(
          "from" + statsName + ".packets_dropped",
          fb303::SUM,
          fb303::RATE),
      toExternalAgentDroppedCount_(
          "to" + statsName + ".packets_dropped",
          fb303::SUM,
          fb303::RATE),
      toExternalAgentSentCount_(
          "to" + statsName + ".packets_sent",
          fb303::SUM,
          fb303::RATE),
      fromExternalAgentSentToHwCount_(
          "from" + statsName + ".packets_sent_to_hw",
          fb303::SUM,
          fb303::RATE) {
  XLOG(INFO) << "PacketStreamHandler initialized";
}

PacketStreamHandler::~PacketStreamHandler() {
  XLOG(INFO) << "PacketStreamHandler destroyed";
}

void PacketStreamHandler::ensureConfigured(folly::StringPiece function) const {
  if (sw_->isFullyConfigured()) {
    return;
  }

  if (!function.empty()) {
    XLOG(DBG1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError(
      "switch is still initializing or is exiting and is not "
      "fully configured yet");
}

// FromExternalAgent (Agent -> Handler -> Switch):
void PacketStreamHandler::processReceivedPacket(
    const std::string& clientId,
    TPacket&& packet) {
  if (!sw_) {
    XLOG(ERR) << "PacketStreamHandler: SwSwitch not available";
    fromExternalAgentDroppedCount_.add(1);
    return;
  }

  ensureConfigured(__func__);
  fromExternalAgentReceivedCount_.add(1);

  XLOG(DBG3) << "PacketStreamHandler: Received packet from client " << clientId
             << " on port " << *packet.l2Port()
             << ", size: " << packet.buf()->size();

  auto portName = *packet.l2Port();
  auto port = sw_->getState()->getPorts()->getPortIf(portName);
  if (!port) {
    XLOG(ERR) << "PacketStreamHandler: Unknown port: " << portName;
    fromExternalAgentDroppedCount_.add(1);
    return;
  }

  auto buf = folly::IOBuf::copyBuffer(*packet.buf());
  auto txPacket = sw_->allocatePacket(buf->computeChainDataLength());
  folly::io::RWPrivateCursor cursor(txPacket->buf());
  cursor.push(buf->data(), buf->length());

  sw_->sendNetworkControlPacketAsync(
      std::move(txPacket), PortDescriptor(port->getID()));
  fromExternalAgentSentToHwCount_.add(1);

  XLOG(DBG4) << "PacketStreamHandler: Sent packet to HW on port " << portName;
}

// toExternalAgent (asic -> Handler -> Agent):
void PacketStreamHandler::handlePacket(std::unique_ptr<RxPacket> pkt) {
  if (!pkt) {
    return;
  }
  if (!clientConnected_.load()) {
    XLOG_EVERY_N(ERR, 100)
        << "PacketStreamHandler: No client connected, dropping packet";
    toExternalAgentDroppedCount_.add(1);
    return;
  }

  PortID portId = pkt->getSrcPort();
  auto portNode = sw_->getState()->getPorts()->getNodeIf(portId);
  if (!portNode) {
    XLOG(ERR) << "PacketStreamHandler: Unknown port ID: " << portId;
    toExternalAgentDroppedCount_.add(1);
    return;
  }

  auto portName = portNode->getName();

  TPacket tpacket;
  tpacket.timestamp() = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
  tpacket.l2Port() = portName;
  tpacket.buf() = pkt->buf()->to<std::string>();

  try {
    auto clientId = connectedClientId_.copy();
    send(clientId, std::move(tpacket));
    toExternalAgentSentCount_.add(1);
  } catch (const std::exception& e) {
    XLOG(ERR) << "PacketStreamHandler: Failed to send to client: " << e.what();
    toExternalAgentDroppedCount_.add(1);
  }
}

// Virtual Hook Implementations

void PacketStreamHandler::clientConnected(const std::string& clientId) {
  *connectedClientId_.wlock() = clientId;
  clientConnected_.store(true);
  XLOG(INFO) << "PacketStreamHandler: Client connected: " << clientId;
}

void PacketStreamHandler::clientDisconnected(const std::string& clientId) {
  clientConnected_.store(false);
  connectedClientId_.wlock()->clear();
  XLOG(INFO) << "PacketStreamHandler: Client disconnected: " << clientId;
}

void PacketStreamHandler::addPort(
    const std::string& clientId,
    const std::string& l2Port) {
  XLOG(DBG2) << "PacketStreamHandler: Port " << l2Port
             << " registered for client " << clientId;
}

void PacketStreamHandler::removePort(
    const std::string& clientId,
    const std::string& l2Port) {
  XLOG(DBG2) << "PacketStreamHandler: Port " << l2Port
             << " unregistered for client " << clientId;
}

} // namespace facebook::fboss
