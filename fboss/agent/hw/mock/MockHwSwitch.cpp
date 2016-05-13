/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockHwSwitch.h"

#include <folly/Memory.h>
#include <folly/dynamic.h>

#include "fboss/agent/hw/mock/MockTxPacket.h"

using folly::make_unique;

namespace facebook { namespace fboss {

MockHwSwitch::MockHwSwitch(MockPlatform *platform) {
}

std::unique_ptr<TxPacket> MockHwSwitch::allocatePacket(uint32_t size) {
  return make_unique<MockTxPacket>(size);
}

bool MockHwSwitch::sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept {
  std::shared_ptr<TxPacket> sp(pkt.release());
  sendPacketSwitched_(sp);
  return true;
}

bool MockHwSwitch::sendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    facebook::fboss::PortID portID) noexcept {
  std::shared_ptr<TxPacket> sp(pkt.release());
  sendPacketOutOfPort_(sp);
  return true;
}

folly::dynamic MockHwSwitch::toFollyDynamic() const {
  return folly::dynamic::object;
}

}} // facebook::fboss
