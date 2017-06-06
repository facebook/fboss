/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockableHwSwitch.h"

#include <folly/Memory.h>
#include <folly/dynamic.h>

#include "fboss/agent/hw/mock/MockTxPacket.h"

using std::make_unique;
using ::testing::Invoke;
using ::testing::_;

namespace facebook { namespace fboss {

MockableHwSwitch::MockableHwSwitch(MockPlatform *platform, HwSwitch* realHw)
    : MockHwSwitch(platform),
      realHw_(realHw) {
  // new functions added to HwSwitch should invoke the corresponding function on
  // the real implementation here.
  ON_CALL(*this, sendPacketSwitched_(_))
    .WillByDefault(Invoke(this, &MockableHwSwitch::sendPacketSwitchedAdaptor));
  ON_CALL(*this, sendPacketOutOfPort_(_, _))
    .WillByDefault(Invoke(this, &MockableHwSwitch::sendPacketOutOfPortAdaptor));
  ON_CALL(*this, init(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::init));
  ON_CALL(*this, stateChangedMock(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::stateChanged));
  ON_CALL(*this, getAndClearNeighborHit(_, _))
    .WillByDefault(Invoke(realHw_, &HwSwitch::getAndClearNeighborHit));
  ON_CALL(*this, toFollyDynamic())
    .WillByDefault(Invoke(realHw_, &HwSwitch::toFollyDynamic));
  ON_CALL(*this, updateStats(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::updateStats));
  ON_CALL(*this, getHighresSamplers(_, _, _))
    .WillByDefault(Invoke(realHw_, &HwSwitch::getHighresSamplers));
  ON_CALL(*this, fetchL2Table(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::fetchL2Table));
  ON_CALL(*this, getPortSpeed(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::getPortSpeed));
  ON_CALL(*this, getMaxPortSpeed(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::getMaxPortSpeed));
  ON_CALL(*this, gracefulExit(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::gracefulExit));
  ON_CALL(*this, initialConfigApplied())
    .WillByDefault(Invoke(realHw_, &HwSwitch::initialConfigApplied));
  ON_CALL(*this, clearWarmBootCache())
    .WillByDefault(Invoke(realHw_, &HwSwitch::clearWarmBootCache));
  ON_CALL(*this, unregisterCallbacks())
    .WillByDefault(Invoke(realHw_, &HwSwitch::unregisterCallbacks));
  ON_CALL(*this, isValidStateUpdate(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::isValidStateUpdate));
  ON_CALL(*this, isPortUp(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::isPortUp));
}

std::unique_ptr<TxPacket> MockableHwSwitch::allocatePacket(uint32_t size) {
  return realHw_->allocatePacket(size);
}

bool MockableHwSwitch::sendPacketSwitchedAdaptor(TxPacket* pkt) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketSwitched(std::move(up));
}

bool MockableHwSwitch::sendPacketOutOfPortAdaptor(
    TxPacket* pkt, PortID port) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketOutOfPort(std::move(up), port);
}

std::shared_ptr<SwitchState>
MockableHwSwitch::stateChanged(const StateDelta& delta) {
  stateChangedMock(delta);
  return delta.newState();
}

}} // facebook::fboss
