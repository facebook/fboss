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
#include <gmock/gmock.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

namespace facebook::fboss {

void MockTestHandle::rxPacket(
    std::unique_ptr<folly::IOBuf> buf,
    PortID srcPort,
    std::optional<VlanID> srcVlan) {
  auto pkt = std::make_unique<MockRxPacket>(std::move(buf));
  pkt->padToLength(68);
  pkt->setSrcPort(srcPort);
  pkt->setSrcVlan(srcVlan);
  getSw()->packetReceived(std::move(pkt));
}

void MockTestHandle::forcePortDown(PortID port) {
  getSw()->linkStateChanged(port, false);
}

void MockTestHandle::forcePortUp(PortID port) {
  getSw()->linkStateChanged(port, true);
}

void MockTestHandle::forcePortFlap(PortID port) {
  forcePortDown(port);
  forcePortUp(port);
}

} // namespace facebook::fboss
