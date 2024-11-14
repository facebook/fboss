/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockTestHandle.h"

#include <folly/io/IOBuf.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

namespace facebook::fboss {

void MockTestHandle::rxPacket(
    std::unique_ptr<folly::IOBuf> buf,
    const PortDescriptor& srcPort,
    std::optional<VlanID> srcVlan) {
  auto len = buf->computeChainDataLength();
  auto pkt = std::make_unique<MockRxPacket>(std::move(buf));
  // The minimum required frame length for ethernet is 64 bytes
  pkt->padToLength(std::max((int)len, 68)); // pad to min packet size if needed
  if (srcPort.isAggregatePort()) {
    pkt->setSrcAggregatePort(srcPort.aggPortID());
  }
  pkt->setSrcPort(PortID(srcPort.intID()));
  pkt->setSrcVlan(srcVlan);
  getSw()->packetReceived(std::move(pkt));
}

void MockTestHandle::forcePortDown(PortID port) {
  getSw()->linkStateChanged(port, false, cfg::PortType::INTERFACE_PORT);
}

void MockTestHandle::forcePortUp(PortID port) {
  getSw()->linkStateChanged(port, true, cfg::PortType::INTERFACE_PORT);
}

void MockTestHandle::forcePortFlap(PortID port) {
  forcePortDown(port);
  forcePortUp(port);
}

} // namespace facebook::fboss
