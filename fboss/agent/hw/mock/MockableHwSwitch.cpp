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
  ON_CALL(*this, sendPacketSwitchedAsync_(_))
      .WillByDefault(
          Invoke(this, &MockableHwSwitch::sendPacketSwitchedAsyncAdaptor));
  ON_CALL(*this, sendPacketOutOfPortAsync_(_, _, _))
      .WillByDefault(
          Invoke(this, &MockableHwSwitch::sendPacketOutOfPortAsyncAdaptor));
  ON_CALL(*this, sendPacketSwitchedSync_(_))
      .WillByDefault(
          Invoke(this, &MockableHwSwitch::sendPacketSwitchedSyncAdaptor));
  ON_CALL(*this, sendPacketOutOfPortSync_(_, _))
      .WillByDefault(
          Invoke(this, &MockableHwSwitch::sendPacketOutOfPortSyncAdaptor));
  ON_CALL(*this, init(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::init));
  ON_CALL(*this, stateChanged(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::stateChanged));
  ON_CALL(*this, getAndClearNeighborHit(_, _))
    .WillByDefault(Invoke(realHw_, &HwSwitch::getAndClearNeighborHit));
  ON_CALL(*this, toFollyDynamic())
    .WillByDefault(Invoke(realHw_, &HwSwitch::toFollyDynamic));
  ON_CALL(*this, updateStats(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::updateStats));
#ifdef OPENNSL_6_4_6_6_ODP
  ON_CALL(*this, fetchL2Table(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::fetchL2Table));
#endif
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
  ON_CALL(*this, clearPortStats(_))
    .WillByDefault(Invoke(realHw_, &HwSwitch::clearPortStats));
}

std::unique_ptr<TxPacket> MockableHwSwitch::allocatePacket(uint32_t size) {
  return realHw_->allocatePacket(size);
}

bool MockableHwSwitch::sendPacketSwitchedAsyncAdaptor(TxPacket* pkt) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketSwitchedAsync(std::move(up));
}

bool MockableHwSwitch::sendPacketOutOfPortAsyncAdaptor(
    TxPacket* pkt,
    PortID port,
    folly::Optional<uint8_t> cos) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketOutOfPortAsync(std::move(up), port, cos);
}

bool MockableHwSwitch::sendPacketSwitchedSyncAdaptor(TxPacket* pkt) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketSwitchedSync(std::move(up));
}

bool MockableHwSwitch::sendPacketOutOfPortSyncAdaptor(
    TxPacket* pkt, PortID port) noexcept {
  std::unique_ptr<TxPacket> up(pkt);
  return realHw_->sendPacketOutOfPortSync(std::move(up), port);
}

}} // facebook::fboss
