// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicHwSwitchHandler.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

MonolinithicHwSwitchHandler::MonolinithicHwSwitchHandler(
    PlatformInitFn initPlatformFn)
    : initPlatformFn_(std::move(initPlatformFn)) {}

MonolinithicHwSwitchHandler::MonolinithicHwSwitchHandler(
    std::unique_ptr<Platform> platform)
    : platform_(std::move(platform)), hw_(platform_->getHwSwitch()) {
  initPlatformData();
}

void MonolinithicHwSwitchHandler::initPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t features) {
  platform_ = initPlatformFn_(std::move(config), features);
  hw_ = platform_->getHwSwitch();
  initPlatformData();
}

HwInitResult MonolinithicHwSwitchHandler::initHw(
    HwSwitchCallback* callback,
    bool failHwCallsOnWarmboot) {
  return hw_->init(
      callback,
      failHwCallsOnWarmboot,
      platform_->getAsic()->getSwitchType(),
      platform_->getAsic()->getSwitchId());
}

void MonolinithicHwSwitchHandler::exitFatal() const {
  return hw_->exitFatal();
}

std::unique_ptr<TxPacket> MonolinithicHwSwitchHandler::allocatePacket(
    uint32_t size) const {
  return hw_->allocatePacket(size);
}

bool MonolinithicHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  return hw_->sendPacketOutOfPortAsync(std::move(pkt), portID, queue);
}

bool MonolinithicHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedSync(std::move(pkt));
}

bool MonolinithicHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return hw_->sendPacketSwitchedAsync(std::move(pkt));
}

bool MonolinithicHwSwitchHandler::isValidStateUpdate(
    const StateDelta& delta) const {
  return hw_->isValidStateUpdate(delta);
}

void MonolinithicHwSwitchHandler::unregisterCallbacks() {
  hw_->unregisterCallbacks();
}

void MonolinithicHwSwitchHandler::gracefulExit(
    folly::dynamic& follySwitchState,
    state::WarmbootState& thriftSwitchState) {
  hw_->gracefulExit(follySwitchState, thriftSwitchState);
}

bool MonolinithicHwSwitchHandler::getAndClearNeighborHit(
    RouterID vrf,
    folly::IPAddress& ip) {
  return hw_->getAndClearNeighborHit(vrf, ip);
}

folly::dynamic MonolinithicHwSwitchHandler::toFollyDynamic() const {
  return hw_->toFollyDynamic();
}

std::optional<uint32_t> MonolinithicHwSwitchHandler::getHwLogicalPortId(
    PortID portID) const {
  auto platformPort = platform_->getPlatformPort(portID);
  return platformPort->getHwLogicalPortId();
}

void MonolinithicHwSwitchHandler::initPlatformData() {
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

void MonolinithicHwSwitchHandler::onHwInitialized(HwSwitchCallback* callback) {
  platform_->onHwInitialized(callback);
}

void MonolinithicHwSwitchHandler::onInitialConfigApplied(
    HwSwitchCallback* callback) {
  platform_->onInitialConfigApplied(callback);
}

void MonolinithicHwSwitchHandler::platformStop() {
  platform_->stop();
}

const AgentConfig* MonolinithicHwSwitchHandler::config() {
  return platform_->config();
}

const AgentConfig* MonolinithicHwSwitchHandler::reloadConfig() {
  return platform_->reloadConfig();
}

bool MonolinithicHwSwitchHandler::transactionsSupported() const {
  return hw_->transactionsSupported();
}

folly::F14FastMap<std::string, HwPortStats>
MonolinithicHwSwitchHandler::getPortStats() const {
  return hw_->getPortStats();
}

std::map<std::string, HwSysPortStats>
MonolinithicHwSwitchHandler::getSysPortStats() const {
  return hw_->getSysPortStats();
}

void MonolinithicHwSwitchHandler::updateStats(SwitchStats* switchStats) {
  return hw_->updateStats(switchStats);
}

std::map<PortID, phy::PhyInfo> MonolinithicHwSwitchHandler::updateAllPhyInfo() {
  return hw_->updateAllPhyInfo();
}

uint64_t MonolinithicHwSwitchHandler::getDeviceWatermarkBytes() const {
  return hw_->getDeviceWatermarkBytes();
}

HwSwitchFb303Stats* MonolinithicHwSwitchHandler::getSwitchStats() const {
  return hw_->getSwitchStats();
}

void MonolinithicHwSwitchHandler::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  hw_->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats>
MonolinithicHwSwitchHandler::getPortAsicPrbsStats(int32_t portId) {
  return hw_->getPortAsicPrbsStats(portId);
}

void MonolinithicHwSwitchHandler::clearPortAsicPrbsStats(int32_t portId) {
  hw_->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial>
MonolinithicHwSwitchHandler::getPortPrbsPolynomials(int32_t portId) {
  return hw_->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState MonolinithicHwSwitchHandler::getPortPrbsState(
    PortID portId) {
  return hw_->getPortPrbsState(portId);
}

std::vector<phy::PrbsLaneStats>
MonolinithicHwSwitchHandler::getPortGearboxPrbsStats(
    int32_t portId,
    phy::Side side) {
  return hw_->getPortGearboxPrbsStats(portId, side);
}

void MonolinithicHwSwitchHandler::clearPortGearboxPrbsStats(
    int32_t portId,
    phy::Side side) {
  hw_->clearPortGearboxPrbsStats(portId, side);
}

void MonolinithicHwSwitchHandler::switchRunStateChanged(
    SwitchRunState newState) {
  hw_->switchRunStateChanged(newState);
}

std::shared_ptr<SwitchState> MonolinithicHwSwitchHandler::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  return transaction ? hw_->stateChangedTransaction(delta)
                     : hw_->stateChanged(delta);
}

CpuPortStats MonolinithicHwSwitchHandler::getCpuPortStats() const {
  return hw_->getCpuPortStats();
}

std::map<PortID, FabricEndpoint>
MonolinithicHwSwitchHandler::getFabricReachability() const {
  return hw_->getFabricReachability();
}

std::vector<PortID> MonolinithicHwSwitchHandler::getSwitchReachability(
    SwitchID switchId) const {
  return hw_->getSwitchReachability(switchId);
}

std::string MonolinithicHwSwitchHandler::getDebugDump() const {
  return hw_->getDebugDump();
}

void MonolinithicHwSwitchHandler::fetchL2Table(
    std::vector<L2EntryThrift>* l2Table) const {
  hw_->fetchL2Table(l2Table);
}

std::string MonolinithicHwSwitchHandler::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) const {
  return hw_->listObjects(types, cached);
}

FabricReachabilityStats
MonolinithicHwSwitchHandler::getFabricReachabilityStats() const {
  return hw_->getFabricReachabilityStats();
}
} // namespace facebook::fboss
