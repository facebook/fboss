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
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiTxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktUtil.h"
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

using facebook::fboss::DeltaFunctions::forEachAdded;

// We need this global SaiSwitch* to support registering SAI callbacks
// which can then use SaiSwitch to do their work. The current callback
// facility in SAI does not support passing user data to come back
// with the callback.
// N.B., if we want to have multiple SaiSwitches in a device with multiple
// cards being managed by one instance of FBOSS, this will need to be
// extended, presumably into an array keyed by switch id.
static SaiSwitch* __gSaiSwitch;

SaiSwitch::SaiSwitch(SaiPlatform* platform, uint32_t featuresDesired)
    : HwSwitch(featuresDesired), platform_(platform) {
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
}

void __gPacketRxCallback(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  __gSaiSwitch->packetRxCallback(
      switch_id, buffer_size, buffer, attr_count, attr_list);
}

void __glinkStateChangedNotification(
    uint32_t count,
    const sai_port_oper_status_notification_t* data) {
  __gSaiSwitch->linkStateChangedCallback(count, data);
}

void __gFdbEventCallback(
    uint32_t count,
    const sai_fdb_event_notification_data_t* data) {
  __gSaiSwitch->fdbEventCallback(count, data);
}

HwInitResult SaiSwitch::init(Callback* callback) noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return initLocked(lock, callback);
}

void SaiSwitch::unregisterCallbacks() noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  unregisterCallbacksLocked(lock);
}

std::shared_ptr<SwitchState> SaiSwitch::stateChanged(const StateDelta& delta) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return stateChangedLocked(lock, delta);
}

bool SaiSwitch::isValidStateUpdate(const StateDelta& delta) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return isValidStateUpdateLocked(lock, delta);
}

std::unique_ptr<TxPacket> SaiSwitch::allocatePacket(uint32_t size) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return allocatePacketLocked(lock, size);
}

bool SaiSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return sendPacketSwitchedAsyncLocked(lock, std::move(pkt));
}

bool SaiSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    folly::Optional<uint8_t> queue) noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return sendPacketOutOfPortAsyncLocked(lock, std::move(pkt), portID, queue);
}

bool SaiSwitch::sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return sendPacketSwitchedSyncLocked(lock, std::move(pkt));
}

bool SaiSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID) noexcept {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return sendPacketOutOfPortSyncLocked(lock, std::move(pkt), portID);
}

void SaiSwitch::updateStats(SwitchStats* switchStats) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  updateStatsLocked(lock, switchStats);
}

void SaiSwitch::fetchL2Table(std::vector<L2EntryThrift>* l2Table) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  fetchL2TableLocked(lock, l2Table);
}

void SaiSwitch::gracefulExit(folly::dynamic& switchState) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  gracefulExitLocked(lock, switchState);
}

folly::dynamic SaiSwitch::toFollyDynamic() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return toFollyDynamicLocked(lock);
}

void SaiSwitch::initialConfigApplied() {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  initialConfigAppliedLocked(lock);
}

void SaiSwitch::switchRunStateChanged(SwitchRunState newState) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  switchRunStateChangedLocked(lock, newState);
}

void SaiSwitch::exitFatal() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  exitFatalLocked(lock);
}

bool SaiSwitch::isPortUp(PortID port) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return isPortUpLocked(lock, port);
}

bool SaiSwitch::getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getAndClearNeighborHitLocked(lock, vrf, ip);
}

void SaiSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  clearPortStatsLocked(lock, ports);
}

cfg::PortSpeed SaiSwitch::getPortMaxSpeed(PortID port) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getPortMaxSpeedLocked(lock, port);
}

/*
 * This signature matches the SAI callback signature and will be invoked
 * immediately by the non-method SAI callback function.
 */
void SaiSwitch::packetRxCallback(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  std::unique_lock<std::mutex> lock(saiSwitchMutex_);
  packetRxCallbackLocked(
      std::move(lock), switch_id, buffer_size, buffer, attr_count, attr_list);
}

void SaiSwitch::linkStateChangedCallback(
    uint32_t count,
    const sai_port_oper_status_notification_t* data) {
  for (auto i = 0; i < count; i++) {
    auto state = data[i].port_state == SAI_PORT_OPER_STATUS_UP ? "up" : "down";
    XLOG(INFO) << "port " << state << " notification received for " << std::hex
               << data[i].port_id;
    // FIXME - accessing port table here is racy - T54107986
    callback_->linkStateChanged(
        managerTable()->portManager().getPortID(data[i].port_id),
        data[i].port_state == SAI_PORT_OPER_STATUS_UP);
  }
}

BootType SaiSwitch::getBootType() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getBootTypeLocked(lock);
}

const SaiManagerTable* SaiSwitch::managerTable() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return managerTableLocked(lock);
}

SaiManagerTable* SaiSwitch::managerTable() {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return managerTableLocked(lock);
}

// Begin Locked functions with actual SaiSwitch functionality

HwInitResult SaiSwitch::initLocked(
    const std::lock_guard<std::mutex>& lock,
    Callback* callback) noexcept {
  HwInitResult ret;
  ret.bootType = BootType::COLD_BOOT;
  bootType_ = BootType::COLD_BOOT;

  sai_api_initialize(0, platform_->getServiceMethodTable());
  SaiApiTable::getInstance()->queryApis();
  managerTable_ = std::make_unique<SaiManagerTable>(platform_);
  switchId_ = managerTable_->switchManager().getSwitchSaiId();

  platform_->initPorts();

  callback_ = callback;
  auto state = std::make_shared<SwitchState>();
  ret.switchState = state;
  __gSaiSwitch = this;
  return ret;
}

void SaiSwitch::packetRxCallbackLocked(
    std::unique_lock<std::mutex>&& lock,
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
  const auto& portManager = managerTable_->portManager();
  const auto& vlanManager = managerTable_->vlanManager();
  PortID swPortId = portManager.getPortID(saiPortId);
  /*
   * TODO: PortID 0 can be a valid port. Throw an exception or check for
   * an invalid port id.
   */
  if (swPortId == 0) {
    lock.unlock();
    return;
  }
  auto swVlanId = vlanManager.getVlanIdByPortId(swPortId);
  auto rxPacket =
      std::make_unique<SaiRxPacket>(buffer_size, buffer, swPortId, swVlanId);
  /*
   * TODO: unlock the mutex here because packet rx cb may end up calling packet
   * tx function in the same thread stack, which also needs a lock.
   */
  lock.unlock();
  callback_->packetReceived(std::move(rxPacket));
}

void SaiSwitch::unregisterCallbacksLocked(
    const std::lock_guard<std::mutex>& /* lock */) noexcept {}

std::shared_ptr<SwitchState> SaiSwitch::stateChangedLocked(
    const std::lock_guard<std::mutex>& lock,
    const StateDelta& delta) {
  managerTableLocked(lock)->portManager().processPortDelta(delta);
  managerTableLocked(lock)->vlanManager().processVlanDelta(
      delta.getVlansDelta());
  managerTableLocked(lock)->routerInterfaceManager().processInterfaceDelta(
      delta);
  managerTableLocked(lock)->neighborManager().processNeighborDelta(delta);
  managerTableLocked(lock)->routeManager().processRouteDelta(delta);
  managerTableLocked(lock)->hostifManager().processControlPlaneDelta(delta);
  return delta.newState();
}

bool SaiSwitch::isValidStateUpdateLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    const StateDelta& /* delta */) const {
  return true;
}

std::unique_ptr<TxPacket> SaiSwitch::allocatePacketLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    uint32_t size) const {
  return std::make_unique<SaiTxPacket>(size);
}

bool SaiSwitch::sendPacketSwitchedAsyncLocked(
    const std::lock_guard<std::mutex>& lock,
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketSwitchedSyncLocked(lock, std::move(pkt));
}

bool SaiSwitch::sendPacketOutOfPortAsyncLocked(
    const std::lock_guard<std::mutex>& lock,
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    folly::Optional<uint8_t> /* queue */) noexcept {
  return sendPacketOutOfPortSyncLocked(lock, std::move(pkt), portID);
}

bool SaiSwitch::sendPacketSwitchedSyncLocked(
    const std::lock_guard<std::mutex>& lock,
    std::unique_ptr<TxPacket> pkt) noexcept {
  /*
  TODO: remove this hack when difference in src and dst mac is no longer
  required pipe line look up causes packet to pass through pipeline and
  subjected to forwarding as if normal packet.in such a case having same source
  & destination mac address, may cause drop. so change destination mac address
  */
  folly::io::Cursor cursor(pkt->buf());
  EthHdr ethHdr{cursor};
  if (ethHdr.getSrcMac() == ethHdr.getDstMac()) {
    auto* pktData = pkt->buf()->writableData();
    /* pktData[6]...pktData[11] is src mac */
    folly::MacAddress hackedMac{"fa:ce:b0:00:00:0c"};
    for (auto i = 0; i < folly::MacAddress::SIZE; i++) {
      pktData[folly::MacAddress::SIZE + i] = hackedMac.bytes()[i];
    }
    XLOG(DBG5) << "hacked packet as source and destination mac are same, "
               << "hacked packet as follows :";
    folly::io::Cursor dump(pkt->buf());
    XLOG(DBG5) << PktUtil::hexDump(dump);
  }

  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_LOOKUP);
  SaiTxPacketTraits::TxAttributes attributes{txType, 0};
  SaiHostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};
  auto& hostifApi = SaiApiTable::getInstance()->hostifApi();
  hostifApi.send(attributes, switchId_, txPacket);
  return true;
}

bool SaiSwitch::sendPacketOutOfPortSyncLocked(
    const std::lock_guard<std::mutex>& lock,
    std::unique_ptr<TxPacket> pkt,
    PortID portID) noexcept {
  auto portHandle =
      managerTableLocked(lock)->portManager().getPortHandle(portID);
  if (!portHandle) {
    throw FbossError("Failed to send packet on invalid port: ", portID);
  }
  /* TODO: this hack is required, sending packet out of port with with pipeline
  bypass, doesn't cause vlan tag stripping. fix this once a pipeline bypass with
  vlan stripping is available. */

  folly::io::Cursor cursor(pkt->buf());
  EthHdr ethHdr{cursor};
  if (!ethHdr.getVlanTags().empty()) {
    CHECK_EQ(ethHdr.getVlanTags().size(), 1)
        << "found more than one vlan tags while sending packet";
    /* hack to strip vlans as pipeline bypass doesn't handle this */
    cursor.reset(pkt->buf());
    XLOG(DBG5) << "strip vlan for packet";
    XLOG(DBG5) << PktUtil::hexDump(cursor);

    constexpr auto kVlanTagSize = 4;
    auto ethPayload = pkt->buf()->clone();
    // trim DA(6), SA(6) & vlan (4)
    ethPayload->trimStart(
        folly::MacAddress::SIZE + folly::MacAddress::SIZE + kVlanTagSize);

    // trim rest of packet except DA(6), SA(6)
    auto totalLength = pkt->buf()->length();
    pkt->buf()->trimEnd(
        totalLength - folly::MacAddress::SIZE - folly::MacAddress::SIZE);

    // append to trimmed ethernet header remaining payload
    pkt->buf()->appendChain(std::move(ethPayload));
    pkt->buf()->coalesce();
    cursor.reset(pkt->buf());
    XLOG(DBG5) << "stripped vlan, new packet";
    XLOG(DBG5) << PktUtil::hexDump(cursor);
  }

  SaiHostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};
  SaiTxPacketTraits::Attributes::EgressPortOrLag egressPort(
      portHandle->port->adapterKey());
  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  SaiTxPacketTraits::TxAttributes attributes{txType, egressPort};
  auto& hostifApi = SaiApiTable::getInstance()->hostifApi();
  hostifApi.send(attributes, switchId_, txPacket);
  return true;
}

void SaiSwitch::updateStatsLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    SwitchStats* /* switchStats */) {}

void SaiSwitch::fetchL2TableLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    std::vector<L2EntryThrift>* l2Table) const {
  auto fdbEntries = getObjectKeys<SaiFdbTraits>(switchId_);
  for (const auto& fdbEntry : fdbEntries) {
    L2EntryThrift entry;
    entry.vlanID = managerTable()->vlanManager().getVlanID(
        VlanSaiId(fdbEntry.bridgeVlanId()));
    entry.mac = fdbEntry.mac().toString();
    auto& fdbApi = SaiApiTable::getInstance()->fdbApi();
    auto saiPortId = fdbApi.getAttribute2(
        fdbEntry, SaiFdbTraits::Attributes::BridgePortId());
    entry.port = managerTable()->portManager().getPortID(saiPortId);
    l2Table->push_back(entry);
  }
}

void SaiSwitch::gracefulExitLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    folly::dynamic& /* switchState */) {}

folly::dynamic SaiSwitch::toFollyDynamicLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  return folly::dynamic::object();
}

void SaiSwitch::initialConfigAppliedLocked(
    const std::lock_guard<std::mutex>& /* lock */) {}

void SaiSwitch::switchRunStateChangedLocked(
    const std::lock_guard<std::mutex>& lock,
    SwitchRunState newState) {
  switch (newState) {
    case SwitchRunState::INITIALIZED: {
      auto& switchApi = SaiApiTable::getInstance()->switchApi();
      if (getFeaturesDesired() & FeaturesDesired::PACKET_RX_DESIRED) {
        switchApi.registerRxCallback(switchId_, __gPacketRxCallback);
      }
      if (getFeaturesDesired() & FeaturesDesired::LINKSCAN_DESIRED) {
        switchApi.registerPortStateChangeCallback(
            switchId_, __glinkStateChangedNotification);
      }
      switchApi.registerFdbEventCallback(switchId_, __gFdbEventCallback);

      /* TODO(T54112206) :remove trapping ARP, NDP & CPU nexthop packets */
      auto& hostifManager = managerTableLocked(lock)->hostifManager();
      for (auto reason : {cfg::PacketRxReason::ARP,
                          cfg::PacketRxReason::NDP,
                          cfg::PacketRxReason::CPU_IS_NHOP}) {
        hostifManager.addHostifTrap(reason, 0);
      }
    } break;
    default:
      break;
  }
}

void SaiSwitch::exitFatalLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {}

bool SaiSwitch::isPortUpLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    PortID /* port */) const {
  return true;
}

cfg::PortSpeed SaiSwitch::getPortMaxSpeedLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    PortID /* port */) const {
  // TODO (srikrishnagopu): Use the read-only attribute
  // SAI_PORT_ATTR_SUPPORTED_SPEED to query the list of supported speeds
  // and return the maximum supported speed.
  return cfg::PortSpeed::HUNDREDG;
}

bool SaiSwitch::getAndClearNeighborHitLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    RouterID /* vrf */,
    folly::IPAddress& /* ip */) {
  return true;
}

void SaiSwitch::clearPortStatsLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    const std::unique_ptr<std::vector<int32_t>>& /* ports */) {}

BootType SaiSwitch::getBootTypeLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  return bootType_;
}

const SaiManagerTable* SaiSwitch::managerTableLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  return managerTable_.get();
}

SaiManagerTable* SaiSwitch::managerTableLocked(
    const std::lock_guard<std::mutex>& /* lock */) {
  return managerTable_.get();
}

void SaiSwitch::fdbEventCallback(
    uint32_t count,
    const sai_fdb_event_notification_data_t* data) {
  fdbEventCallbackLocked(count, data);
}

void SaiSwitch::fdbEventCallbackLocked(
    uint32_t /*count*/,
    const sai_fdb_event_notification_data_t* /*data*/) {
  // TODO  - program macs from learn events to FDB
}

} // namespace fboss
} // namespace facebook
