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
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace facebook::fboss {

MockHwSwitch::MockHwSwitch(MockPlatform* platform) : platform_(platform) {
  // We need to delete the memory allocated by the packet. Ideally, we
  // could pass a smart pointer to these functions, but gmock does not
  // support move-only types and if we pass in a shared_ptr we cannot
  // delegate to a function that takes a unique_ptr. This is
  // definitely hacky, but it makes the interface more flexible.
  ON_CALL(*this, sendPacketSwitchedAsync_(_))
      .WillByDefault(Invoke([=](TxPacket* pkt) -> bool {
        delete pkt;
        return true;
      }));
  ON_CALL(*this, sendPacketOutOfPortAsync_(_, _, _))
      .WillByDefault(Invoke(
          [=](TxPacket* pkt,
              PortID /*port*/,
              std::optional<uint8_t> /* cos */) -> bool {
            delete pkt;
            return true;
          }));
  ON_CALL(*this, sendPacketSwitchedSync_(_))
      .WillByDefault(Invoke([=](TxPacket* pkt) -> bool {
        delete pkt;
        return true;
      }));
  ON_CALL(*this, sendPacketOutOfPortSync_(_, _, _))
      .WillByDefault(Invoke(
          [=](TxPacket* pkt,
              PortID /*port*/,
              std::optional<uint8_t> /* cos */) -> bool {
            delete pkt;
            return true;
          }));
  ON_CALL(*this, stateChangedImpl(_))
      .WillByDefault(
          Invoke([](const StateDelta& delta) { return delta.newState(); }));
  ON_CALL(*this, stateChangedTransaction(_))
      .WillByDefault(
          Invoke([](const StateDelta& delta) { return delta.newState(); }));
  ON_CALL(*this, transactionsSupported()).WillByDefault(Return(false));
  ON_CALL(*this, isValidStateUpdate(_)).WillByDefault(Return(true));
  ON_CALL(*this, getAndClearNeighborHit(_, _)).WillByDefault(Return(true));
}

std::unique_ptr<TxPacket> MockHwSwitch::allocatePacket(uint32_t size) const {
  return make_unique<MockTxPacket>(size);
}

bool MockHwSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketSwitchedAsync_(raw);
  return true;
}

bool MockHwSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    facebook::fboss::PortID portID,
    std::optional<uint8_t> queue) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketOutOfPortAsync_(raw, portID, queue);
  return true;
}

bool MockHwSwitch::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketSwitchedSync_(raw);
  return true;
}

void MockHwSwitch::printDiagCmd(const std::string& /*cmd*/) const {
  // TODO: Needs to be implemented.
}

bool MockHwSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> pkt,
    facebook::fboss::PortID portID,
    std::optional<uint8_t> queue) noexcept {
  TxPacket* raw(pkt.release());
  sendPacketOutOfPortSync_(raw, portID, queue);
  return true;
}

} // namespace facebook::fboss
