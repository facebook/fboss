// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicHwSwitchHandlerDeprecated.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

MonolinithicHwSwitchHandlerDeprecated::MonolinithicHwSwitchHandlerDeprecated(
    Platform* platform)
    : platform_(platform), hw_(platform_->getHwSwitch()) {
  initPlatformData();
}

void MonolinithicHwSwitchHandlerDeprecated::exitFatal() const {
  return hw_->exitFatal();
}

std::unique_ptr<TxPacket> MonolinithicHwSwitchHandlerDeprecated::allocatePacket(
    uint32_t size) const {
  return hw_->allocatePacket(size);
}

bool MonolinithicHwSwitchHandlerDeprecated::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  return hw_->sendPacketOutOfPortAsync(std::move(pkt), portID, queue);
}

bool MonolinithicHwSwitchHandlerDeprecated::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedSync(std::move(pkt));
}

bool MonolinithicHwSwitchHandlerDeprecated::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedAsync(std::move(pkt));
}

bool MonolinithicHwSwitchHandlerDeprecated::isValidStateUpdate(
    const StateDelta& delta) const {
  return hw_->isValidStateUpdate(delta);
}

void MonolinithicHwSwitchHandlerDeprecated::unregisterCallbacks() {
  hw_->unregisterCallbacks();
}

void MonolinithicHwSwitchHandlerDeprecated::gracefulExit(
    state::WarmbootState& thriftSwitchState) {
  hw_->gracefulExit(thriftSwitchState);
}

bool MonolinithicHwSwitchHandlerDeprecated::getAndClearNeighborHit(
    RouterID vrf,
    folly::IPAddress& ip) {
  return hw_->getAndClearNeighborHit(vrf, ip);
}

folly::dynamic MonolinithicHwSwitchHandlerDeprecated::toFollyDynamic() const {
  return hw_->toFollyDynamic();
}

std::optional<uint32_t>
MonolinithicHwSwitchHandlerDeprecated::getHwLogicalPortId(PortID portID) const {
  auto platformPort = platform_->getPlatformPort(portID);
  return platformPort->getHwLogicalPortId();
}

void MonolinithicHwSwitchHandlerDeprecated::initPlatformData() {
  platformData_.volatileStateDir = platform_->getVolatileStateDir();
  platformData_.persistentStateDir = platform_->getPersistentStateDir();
  platformData_.crashSwitchStateFile = platform_->getCrashSwitchStateFile();
  platformData_.crashThriftSwitchStateFile =
      platform_->getCrashThriftSwitchStateFile();
  platformData_.warmBootDir = platform_->getWarmBootDir();
  platformData_.crashBadStateUpdateDir = platform_->getCrashBadStateUpdateDir();
  platformData_.crashBadStateUpdateOldStateFile =
      platform_->getCrashBadStateUpdateOldStateFile();
  platformData_.crashBadStateUpdateNewStateFile =
      platform_->getCrashBadStateUpdateNewStateFile();
  platformData_.runningConfigDumpFile = platform_->getRunningConfigDumpFile();
  platformData_.supportsAddRemovePort = platform_->supportsAddRemovePort();
}

void MonolinithicHwSwitchHandlerDeprecated::onHwInitialized(
    HwSwitchCallback* callback) {
  platform_->onHwInitialized(callback);
}

void MonolinithicHwSwitchHandlerDeprecated::onInitialConfigApplied(
    HwSwitchCallback* callback) {
  platform_->onInitialConfigApplied(callback);
}

void MonolinithicHwSwitchHandlerDeprecated::platformStop() {
  platform_->stop();
}

const AgentConfig* MonolinithicHwSwitchHandlerDeprecated::config() {
  return platform_->config();
}

const AgentConfig* MonolinithicHwSwitchHandlerDeprecated::reloadConfig() {
  return platform_->reloadConfig();
}

bool MonolinithicHwSwitchHandlerDeprecated::transactionsSupported() const {
  return hw_->transactionsSupported();
}

folly::F14FastMap<std::string, HwPortStats>
MonolinithicHwSwitchHandlerDeprecated::getPortStats() const {
  return hw_->getPortStats();
}

std::map<std::string, HwSysPortStats>
MonolinithicHwSwitchHandlerDeprecated::getSysPortStats() const {
  return hw_->getSysPortStats();
}

void MonolinithicHwSwitchHandlerDeprecated::updateStats(
    SwitchStats* switchStats) {
  return hw_->updateStats(switchStats);
}

std::map<PortID, phy::PhyInfo>
MonolinithicHwSwitchHandlerDeprecated::updateAllPhyInfo() {
  return hw_->updateAllPhyInfo();
}

uint64_t MonolinithicHwSwitchHandlerDeprecated::getDeviceWatermarkBytes()
    const {
  return hw_->getDeviceWatermarkBytes();
}

HwSwitchFb303Stats* MonolinithicHwSwitchHandlerDeprecated::getSwitchStats()
    const {
  return hw_->getSwitchStats();
}

void MonolinithicHwSwitchHandlerDeprecated::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  hw_->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats>
MonolinithicHwSwitchHandlerDeprecated::getPortAsicPrbsStats(int32_t portId) {
  return hw_->getPortAsicPrbsStats(portId);
}

void MonolinithicHwSwitchHandlerDeprecated::clearPortAsicPrbsStats(
    int32_t portId) {
  hw_->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial>
MonolinithicHwSwitchHandlerDeprecated::getPortPrbsPolynomials(int32_t portId) {
  return hw_->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState
MonolinithicHwSwitchHandlerDeprecated::getPortPrbsState(PortID portId) {
  return hw_->getPortPrbsState(portId);
}

void MonolinithicHwSwitchHandlerDeprecated::switchRunStateChanged(
    SwitchRunState newState) {
  hw_->switchRunStateChanged(newState);
}

std::shared_ptr<SwitchState>
MonolinithicHwSwitchHandlerDeprecated::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  return transaction ? hw_->stateChangedTransaction(delta)
                     : hw_->stateChanged(delta);
}

CpuPortStats MonolinithicHwSwitchHandlerDeprecated::getCpuPortStats() const {
  return hw_->getCpuPortStats();
}

std::map<PortID, FabricEndpoint>
MonolinithicHwSwitchHandlerDeprecated::getFabricReachability() const {
  return hw_->getFabricReachability();
}

std::vector<PortID>
MonolinithicHwSwitchHandlerDeprecated::getSwitchReachability(
    SwitchID switchId) const {
  return hw_->getSwitchReachability(switchId);
}

std::string MonolinithicHwSwitchHandlerDeprecated::getDebugDump() const {
  return hw_->getDebugDump();
}

void MonolinithicHwSwitchHandlerDeprecated::fetchL2Table(
    std::vector<L2EntryThrift>* l2Table) const {
  hw_->fetchL2Table(l2Table);
}

std::string MonolinithicHwSwitchHandlerDeprecated::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) const {
  return hw_->listObjects(types, cached);
}

FabricReachabilityStats
MonolinithicHwSwitchHandlerDeprecated::getFabricReachabilityStats() const {
  return hw_->getFabricReachabilityStats();
}

bool MonolinithicHwSwitchHandlerDeprecated::needL2EntryForNeighbor() const {
  return hw_->needL2EntryForNeighbor();
}

fsdb::OperDelta MonolinithicHwSwitchHandlerDeprecated::stateChanged(
    const fsdb::OperDelta& delta,
    bool transaction) {
  return transaction ? hw_->stateChangedTransaction(delta)
                     : hw_->stateChanged(delta);
}
} // namespace facebook::fboss
