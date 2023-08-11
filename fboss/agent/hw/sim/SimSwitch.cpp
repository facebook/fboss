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

#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/hw/mock/MockTxPacket.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/dynamic.h>

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;

namespace facebook::fboss {

SimSwitch::SimSwitch(SimPlatform* platform, uint32_t numPorts)
    : platform_(platform), numPorts_(numPorts) {}

HwInitResult SimSwitch::initImpl(
    HwSwitchCallback* callback,
    BootType /*bootType*/,
    bool /*failHwCallsOnWarmboot*/) {
  HwInitResult ret;
  callback_ = callback;

  auto state = make_shared<SwitchState>();
  for (uint32_t idx = 1; idx <= numPorts_; ++idx) {
    auto name = folly::to<string>("Port", idx);
    state::PortFields portFields;
    portFields.portId() = PortID(idx);
    portFields.portName() = name;
    portFields.portType() = cfg::PortType::INTERFACE_PORT;
    auto port = std::make_shared<Port>(std::move(portFields));
    state->getPorts()->addNode(
        port, HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID{0}})));
  }
  bootType_ = BootType::COLD_BOOT;
  ret.bootType = bootType_;
  ret.switchState = state;
  return ret;
}

std::shared_ptr<SwitchState> SimSwitch::stateChangedImpl(
    const StateDelta& delta) {
  // TODO
  return delta.newState();
}

void SimSwitch::printDiagCmd(const std::string& /*cmd*/) const {
  // TODO: Needs to be implemented.
}

std::unique_ptr<TxPacket> SimSwitch::allocatePacket(uint32_t size) const {
  return make_unique<MockTxPacket>(size);
}

bool SimSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO
  ++txCount_;
  return true;
}

bool SimSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> /*pkt*/,
    PortID /*portID*/,
    std::optional<uint8_t> /* queue */) noexcept {
  // TODO
  ++txCount_;
  return true;
}

bool SimSwitch::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO
  ++txCount_;
  return true;
}

bool SimSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> /*pkt*/,
    PortID /*portID*/,
    std::optional<uint8_t> /* queue */) noexcept {
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

} // namespace facebook::fboss
