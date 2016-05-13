/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sim/SimSwitch.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/hw/mock/MockTxPacket.h"

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/Memory.h>

using folly::make_unique;
using std::make_shared;
using std::shared_ptr;
using std::string;

namespace facebook { namespace fboss {

SimSwitch::SimSwitch(SimPlatform* platform, uint32_t numPorts)
  : numPorts_(numPorts) {
}

HwInitResult SimSwitch::init(HwSwitch::Callback* callback) {
  HwInitResult ret;
  callback_ = callback;

  auto state = make_shared<SwitchState>();
  for (uint32_t idx = 1; idx <= numPorts_; ++idx) {
    auto name = folly::to<string>("Port", idx);
    state->registerPort(PortID(idx), name);
  }
  ret.bootType = BootType::COLD_BOOT;
  ret.switchState = state;
  return ret;
}

void SimSwitch::stateChanged(const StateDelta& delta) {
  // TODO
}

std::unique_ptr<TxPacket> SimSwitch::allocatePacket(uint32_t size) {
  return make_unique<MockTxPacket>(size);
}

bool SimSwitch::sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept {
  // TODO
  ++txCount_;
  return true;
}

bool SimSwitch::sendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    PortID portID) noexcept {
  // TODO
  ++txCount_;
  return true;
}
void SimSwitch::injectPacket(std::unique_ptr<RxPacket> pkt) {
  callback_->packetReceived(std::move(pkt));
}

folly::dynamic SimSwitch::toFollyDynamic() const {
  return folly::dynamic::object;
}

}} // facebook::fboss
