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

using std::make_unique;
using ::testing::Invoke;
using ::testing::_;

namespace facebook { namespace fboss {

MockHwSwitch::MockHwSwitch(MockPlatform *platform) :
    platform_(platform) {
  // We need to delete the memory allocated by the packet. Ideally, we
  // could pass a smart pointer to these functions, but gmock does not
  // support move-only types and if we pass in a shared_ptr we cannot
  // delegate to a function that takes a unique_ptr. This is
  // definitely hacky, but it makes the interface more flexible.
  ON_CALL(*this, sendPacketSwitched_(_))
    .WillByDefault(Invoke([=](TxPacket* pkt) -> bool {
          delete pkt; return true; } ));
  ON_CALL(*this, sendPacketOutOfPort_(_, _))
    .WillByDefault(Invoke([=](TxPacket* pkt, PortID port) -> bool {
          delete pkt; return true; } ));
}

std::unique_ptr<TxPacket> MockHwSwitch::allocatePacket(uint32_t size) {
  return make_unique<MockTxPacket>(size);
}

bool MockHwSwitch::sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketSwitched_(raw);
  return true;
}

bool MockHwSwitch::sendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    facebook::fboss::PortID portID) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketOutOfPort_(raw, portID);
  return true;
}

std::shared_ptr<SwitchState>
MockHwSwitch::stateChanged(const StateDelta& delta) {
    stateChangedMock(delta);
    return delta.newState();
}

}} // facebook::fboss
