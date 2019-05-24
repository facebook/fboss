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
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiTxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <iomanip>
#include <memory>
#include <sstream>

extern "C" {
#include <sai.h>
}
namespace facebook {
namespace fboss {

static SaiSwitch* hwSwitch;

void packetRxCallback(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_object_id_t saiPortId = 0;
  for (auto index = 0; index < attr_count; index++) {
    const sai_attribute_t* attr = &attr_list[index];
    switch (attr->id) {
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT:
        saiPortId = attr->value.oid;
        break;
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG:
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID:
        break;
      default:
        XLOG(INFO) << "invalid attribute received";
    }
  }
  CHECK_NE(saiPortId, 0);
  auto portId = hwSwitch->managerTable()->portManager().getPortID(saiPortId);
  auto port = hwSwitch->managerTable()->portManager().getPort(portId);
  auto vlanId = port->getPortVlan();
  auto rxPacket =
      std::make_unique<SaiRxPacket>(buffer_size, buffer, portId, vlanId);
  hwSwitch->packetReceived(std::move(rxPacket));
}

using facebook::fboss::DeltaFunctions::forEachAdded;

HwInitResult SaiSwitch::init(Callback* callback) noexcept {
  HwInitResult ret;
  ret.bootType = BootType::COLD_BOOT;
  bootType_ = BootType::COLD_BOOT;

  sai_api_initialize(0, platform_->getServiceMethodTable());
  saiApiTable_ = std::make_unique<SaiApiTable>();
  managerTable_ =
      std::make_unique<SaiManagerTable>(saiApiTable_.get(), platform_);
  callback_ = callback;

  auto state = std::make_shared<SwitchState>();
  ret.switchState = state;
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  auto& switchApi = saiApiTable_->switchApi();
  switchApi.registerRxCallback(switchId, packetRxCallback);
  hwSwitch = this;
  return ret;
}

void SaiSwitch::packetReceived(std::unique_ptr<SaiRxPacket> rxPacket) {
  callback_->packetReceived(std::move(rxPacket));
}

void SaiSwitch::unregisterCallbacks() noexcept {}
std::shared_ptr<SwitchState> SaiSwitch::stateChanged(const StateDelta& delta) {
  managerTable_->vlanManager().processVlanDelta(delta.getVlansDelta());
  managerTable_->routerInterfaceManager().processInterfaceDelta(delta);
  managerTable_->neighborManager().processNeighborDelta(delta);
  managerTable_->routeManager().processRouteDelta(delta);
  return delta.newState();
}

bool SaiSwitch::isValidStateUpdate(const StateDelta& /* delta */) const {
  return true;
}

std::unique_ptr<TxPacket> SaiSwitch::allocatePacket(uint32_t size) {
  return std::make_unique<SaiTxPacket>(size);
}

bool SaiSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketSwitchedSync(std::move(pkt));
}

bool SaiSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> /* pkt */,
    PortID /* portID */,
    folly::Optional<uint8_t> /* queue */) noexcept {
  return true;
}

bool SaiSwitch::sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept {
  HostifApiParameters::TxPacketAttributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_LOOKUP);
  HostifApiParameters::TxPacketAttributes attributes{{txType, 0}};
  HostifApiParameters::HostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  auto& hostifPacketApi = saiApiTable_->hostifPacketApi();
  hostifPacketApi.send(attributes.attrs(), switchId, txPacket);
  return true;
}

bool SaiSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID) noexcept {
  auto port = managerTable_->portManager().getPort(portID);
  if (!port) {
    throw FbossError("Failed to send packet on invalid port: ", portID);
  }
  HostifApiParameters::HostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};
  HostifApiParameters::TxPacketAttributes::EgressPortOrLag egressPort(
      port->id());
  HostifApiParameters::TxPacketAttributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  HostifApiParameters::TxPacketAttributes attributes{{txType, egressPort}};
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  auto& hostifPacketApi = saiApiTable_->hostifPacketApi();
  hostifPacketApi.send(attributes.attrs(), switchId, txPacket);
  return true;
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

cfg::PortSpeed SaiSwitch::getPortMaxSpeed(PortID /* port */) const {
  // TODO (srikrishnagopu): Use the read-only attribute
  // SAI_PORT_ATTR_SUPPORTED_SPEED to query the list of supported speeds
  // and return the maximum supported speed.
  return cfg::PortSpeed::HUNDREDG;
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
