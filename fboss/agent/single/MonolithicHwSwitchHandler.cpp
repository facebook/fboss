// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicHwSwitchHandler.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

MonolithicHwSwitchHandler::MonolithicHwSwitchHandler(
    Platform* platform,
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)
    : HwSwitchHandler(switchId, info),
      platform_(platform),
      hw_(platform_->getHwSwitch()) {
  initPlatformData();
}

void MonolithicHwSwitchHandler::exitFatal() const {
  return hw_->exitFatal();
}

std::unique_ptr<TxPacket> MonolithicHwSwitchHandler::allocatePacket(
    uint32_t size) const {
  return hw_->allocatePacket(size);
}

bool MonolithicHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  return hw_->sendPacketOutOfPortAsync(std::move(pkt), portID, queue);
}

bool MonolithicHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedSync(std::move(pkt));
}

bool MonolithicHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedAsync(std::move(pkt));
}

bool MonolithicHwSwitchHandler::isValidStateUpdate(
    const StateDelta& delta) const {
  return hw_->isValidStateUpdate(delta);
}

void MonolithicHwSwitchHandler::unregisterCallbacks() {
  hw_->unregisterCallbacks();
}

void MonolithicHwSwitchHandler::gracefulExit(
    state::WarmbootState& thriftSwitchState) {
  hw_->gracefulExit(thriftSwitchState);
}

bool MonolithicHwSwitchHandler::getAndClearNeighborHit(
    RouterID vrf,
    folly::IPAddress& ip) {
  return hw_->getAndClearNeighborHit(vrf, ip);
}

folly::dynamic MonolithicHwSwitchHandler::toFollyDynamic() const {
  return hw_->toFollyDynamic();
}

std::optional<uint32_t> MonolithicHwSwitchHandler::getHwLogicalPortId(
    PortID portID) const {
  auto platformPort = platform_->getPlatformPort(portID);
  return platformPort->getHwLogicalPortId();
}

void MonolithicHwSwitchHandler::initPlatformData() {
  platformData_.volatileStateDir =
      platform_->getDirectoryUtil()->getVolatileStateDir();
  platformData_.persistentStateDir =
      platform_->getDirectoryUtil()->getPersistentStateDir();
  platformData_.crashSwitchStateFile =
      platform_->getDirectoryUtil()->getCrashSwitchStateFile();
  platformData_.crashThriftSwitchStateFile =
      platform_->getDirectoryUtil()->getCrashThriftSwitchStateFile();
  platformData_.warmBootDir = platform_->getDirectoryUtil()->getWarmBootDir();
  platformData_.crashBadStateUpdateDir =
      platform_->getDirectoryUtil()->getCrashBadStateUpdateDir();
  platformData_.crashBadStateUpdateOldStateFile =
      platform_->getDirectoryUtil()->getCrashBadStateUpdateOldStateFile();
  platformData_.crashBadStateUpdateNewStateFile =
      platform_->getDirectoryUtil()->getCrashBadStateUpdateNewStateFile();
  platformData_.runningConfigDumpFile =
      platform_->getDirectoryUtil()->getRunningConfigDumpFile();
  platformData_.supportsAddRemovePort = platform_->supportsAddRemovePort();
}

void MonolithicHwSwitchHandler::onHwInitialized(HwSwitchCallback* callback) {
  platform_->onHwInitialized(callback);
}

void MonolithicHwSwitchHandler::onInitialConfigApplied(
    HwSwitchCallback* callback) {
  platform_->onInitialConfigApplied(callback);
}

void MonolithicHwSwitchHandler::platformStop() {
  platform_->stop();
}

const AgentConfig* MonolithicHwSwitchHandler::config() {
  return platform_->config();
}

const AgentConfig* MonolithicHwSwitchHandler::reloadConfig() {
  return platform_->reloadConfig();
}

bool MonolithicHwSwitchHandler::transactionsSupported() const {
  return hw_->transactionsSupported();
}

folly::F14FastMap<std::string, HwPortStats>
MonolithicHwSwitchHandler::getPortStats() const {
  return hw_->getPortStats();
}

std::map<std::string, HwSysPortStats>
MonolithicHwSwitchHandler::getSysPortStats() const {
  return hw_->getSysPortStats();
}

void MonolithicHwSwitchHandler::updateStats(SwitchStats* switchStats) {
  return hw_->updateStats(switchStats);
}

std::map<PortID, phy::PhyInfo> MonolithicHwSwitchHandler::updateAllPhyInfo() {
  return hw_->updateAllPhyInfo();
}

uint64_t MonolithicHwSwitchHandler::getDeviceWatermarkBytes() const {
  return hw_->getDeviceWatermarkBytes();
}

HwSwitchFb303Stats* MonolithicHwSwitchHandler::getSwitchStats() const {
  return hw_->getSwitchStats();
}

void MonolithicHwSwitchHandler::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  hw_->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats> MonolithicHwSwitchHandler::getPortAsicPrbsStats(
    int32_t portId) {
  return hw_->getPortAsicPrbsStats(portId);
}

void MonolithicHwSwitchHandler::clearPortAsicPrbsStats(int32_t portId) {
  hw_->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial>
MonolithicHwSwitchHandler::getPortPrbsPolynomials(int32_t portId) {
  return hw_->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState MonolithicHwSwitchHandler::getPortPrbsState(
    PortID portId) {
  return hw_->getPortPrbsState(portId);
}

void MonolithicHwSwitchHandler::switchRunStateChanged(SwitchRunState newState) {
  hw_->switchRunStateChanged(newState);
}

std::shared_ptr<SwitchState> MonolithicHwSwitchHandler::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  return transaction ? hw_->stateChangedTransaction(delta)
                     : hw_->stateChanged(delta);
}

CpuPortStats MonolithicHwSwitchHandler::getCpuPortStats() const {
  return hw_->getCpuPortStats();
}

std::map<PortID, FabricEndpoint>
MonolithicHwSwitchHandler::getFabricReachability() const {
  return hw_->getFabricReachability();
}

std::vector<PortID> MonolithicHwSwitchHandler::getSwitchReachability(
    SwitchID switchId) const {
  return hw_->getSwitchReachability(switchId);
}

std::string MonolithicHwSwitchHandler::getDebugDump() const {
  return hw_->getDebugDump();
}

void MonolithicHwSwitchHandler::fetchL2Table(
    std::vector<L2EntryThrift>* l2Table) const {
  hw_->fetchL2Table(l2Table);
}

std::string MonolithicHwSwitchHandler::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) const {
  return hw_->listObjects(types, cached);
}

FabricReachabilityStats MonolithicHwSwitchHandler::getFabricReachabilityStats()
    const {
  return hw_->getFabricReachabilityStats();
}

bool MonolithicHwSwitchHandler::needL2EntryForNeighbor() const {
  return hw_->needL2EntryForNeighbor();
}

fsdb::OperDelta MonolithicHwSwitchHandler::stateChanged(
    const fsdb::OperDelta& delta,
    bool transaction) {
  return transaction ? hw_->stateChangedTransaction(delta)
                     : hw_->stateChanged(delta);
}

multiswitch::StateOperDelta MonolithicHwSwitchHandler::getNextStateOperDelta() {
  throw FbossError("Not supported");
}

void MonolithicHwSwitchHandler::cancelOperDeltaRequest() {
  throw FbossError("Not supported");
}

} // namespace facebook::fboss
