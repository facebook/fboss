/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include <memory>

namespace facebook {
namespace fboss {

using facebook::fboss::DeltaFunctions::forEachAdded;

class SaiTxPacket : public TxPacket {
 public:
  SaiTxPacket() {}
};

HwInitResult SaiSwitch::init(Callback* /* callback */) noexcept {
  saiApiTable_ = std::make_unique<SaiApiTable>();
  managerTable_ = std::make_unique<SaiManagerTable>(saiApiTable_.get());
  return HwInitResult();
}
void SaiSwitch::unregisterCallbacks() noexcept {}
std::shared_ptr<SwitchState> SaiSwitch::stateChanged(
    const StateDelta& /* delta */) {
  return std::make_shared<SwitchState>();
}
bool SaiSwitch::isValidStateUpdate(const StateDelta& /* delta */) const {
  return true;
}
std::unique_ptr<TxPacket> SaiSwitch::allocatePacket(uint32_t /* size */) {
  return std::make_unique<SaiTxPacket>();
}
bool SaiSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> /* pkt */) noexcept {
  return false;
}
bool SaiSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> /* pkt */,
    PortID /* portID */,
    folly::Optional<uint8_t> /* cos */) noexcept {
  return false;
}
bool SaiSwitch::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> /* pkt */) noexcept {
  return false;
}
bool SaiSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> /* pkt */,
    PortID portID) noexcept {
  return false;
}
void SaiSwitch::updateStats(SwitchStats* /* switchStats */) {}
void SaiSwitch::fetchL2Table(std::vector<L2EntryThrift>* /* l2Table */) {}
void SaiSwitch::gracefulExit(folly::dynamic& /* switchState */) {}
folly::dynamic SaiSwitch::toFollyDynamic() const {
  return folly::dynamic::object();
}
void SaiSwitch::initialConfigApplied() {}
void SaiSwitch::clearWarmBootCache() {}
void SaiSwitch::exitFatal() const {}
bool SaiSwitch::isPortUp(PortID /* port */) const {
  return false;
}
bool SaiSwitch::getAndClearNeighborHit(
    RouterID /* vrf */,
    folly::IPAddress& /* ip */) {
  return true;
}
void SaiSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& /* ports */) {}

} // namespace fboss
} // namespace facebook
