// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiHwSwitchHandler.h"
#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"

namespace facebook::fboss {

MultiHwSwitchHandler::MultiHwSwitchHandler(
    const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn) {
  for (auto entry : switchInfoMap) {
    hwSwitchSyncers_.emplace(
        SwitchID(entry.first),
        hwSwitchHandlerInitFn(SwitchID(entry.first), entry.second));
  }
}

MultiHwSwitchHandler::~MultiHwSwitchHandler() {
  stop();
}

void MultiHwSwitchHandler::start() {
  if (!stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->start();
  }
  stopped_.store(false);
}

void MultiHwSwitchHandler::stop() {
  if (stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second.reset();
  }
  stopped_.store(true);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandler::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  if (stopped_.load()) {
    throw FbossError("multi hw switch syncer not started");
  }
  // TODO: support more than one switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  auto iter = hwSwitchSyncers_.begin();
  auto switchId = iter->first;
  auto update = HwSwitchStateUpdate(delta, transaction);
  auto future = stateChanged(switchId, update);
  return getStateUpdateResult(switchId, std::move(future));
}

folly::Future<std::shared_ptr<SwitchState>> MultiHwSwitchHandler::stateChanged(
    SwitchID switchId,
    const HwSwitchStateUpdate& update) {
  auto iter = hwSwitchSyncers_.find(switchId);
  if (iter == hwSwitchSyncers_.end()) {
    throw FbossError("hw switch syncer for switch id ", switchId, " not found");
  }
  return iter->second->stateChanged(update);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandler::getStateUpdateResult(
    SwitchID switchId,
    folly::Future<std::shared_ptr<SwitchState>>&& future) {
  auto result = std::move(future).getTry();
  if (result.hasException()) {
    XLOG(ERR) << "Failed to get state update result for switch id " << switchId
              << ":" << result.exception().what();
    result.exception().throw_exception();
  }
  CHECK(result.hasValue());
  return result.value();
}

HwSwitchHandler* MultiHwSwitchHandler::getHwSwitchHandler(SwitchID switchId) {
  auto handler = hwSwitchSyncers_.find(switchId);
  if (handler == hwSwitchSyncers_.end()) {
    throw FbossError(
        "No hw switch handler for switch id: ", switchId, " found!");
  }
  return handler->second.get();
}

void MultiHwSwitchHandler::unregisterCallbacks() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->unregisterCallbacks();
  }
}

void MultiHwSwitchHandler::exitFatal() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->exitFatal();
  }
}

bool MultiHwSwitchHandler::isValidStateUpdate(const StateDelta& delta) {
  for (auto& entry : hwSwitchSyncers_) {
    if (!entry.second->isValidStateUpdate(delta)) {
      return false;
    }
  }
  return true;
}

void MultiHwSwitchHandler::gracefulExit(
    state::WarmbootState& thriftSwitchState) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->gracefulExit(thriftSwitchState);
  }
}

bool MultiHwSwitchHandler::getAndClearNeighborHit(
    RouterID vrf,
    folly::IPAddress& ip) {
  for (auto& entry : hwSwitchSyncers_) {
    if (entry.second->getAndClearNeighborHit(vrf, ip)) {
      return true;
    }
  }
  return false;
}

folly::dynamic MultiHwSwitchHandler::toFollyDynamic() {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->toFollyDynamic();
}

const PlatformData& MultiHwSwitchHandler::getPlatformData() const {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPlatformData();
}

bool MultiHwSwitchHandler::transactionsSupported() {
  for (auto& entry : hwSwitchSyncers_) {
    if (!entry.second->transactionsSupported()) {
      return false;
    }
  }
  return true;
}

HwSwitchFb303Stats* MultiHwSwitchHandler::getSwitchStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSwitchStats();
}

folly::F14FastMap<std::string, HwPortStats>
MultiHwSwitchHandler::getPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortStats();
}

CpuPortStats MultiHwSwitchHandler::getCpuPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getCpuPortStats();
}

std::map<std::string, HwSysPortStats> MultiHwSwitchHandler::getSysPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSysPortStats();
}

void MultiHwSwitchHandler::updateStats(SwitchStats* switchStats) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->updateStats(switchStats);
}

std::map<PortID, phy::PhyInfo> MultiHwSwitchHandler::updateAllPhyInfo() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->updateAllPhyInfo();
}

uint64_t MultiHwSwitchHandler::getDeviceWatermarkBytes() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getDeviceWatermarkBytes();
}

void MultiHwSwitchHandler::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats> MultiHwSwitchHandler::getPortAsicPrbsStats(
    int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortAsicPrbsStats(portId);
}

void MultiHwSwitchHandler::clearPortAsicPrbsStats(int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial> MultiHwSwitchHandler::getPortPrbsPolynomials(
    int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState MultiHwSwitchHandler::getPortPrbsState(PortID portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortPrbsState(portId);
}

void MultiHwSwitchHandler::switchRunStateChanged(SwitchRunState newState) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->switchRunStateChanged(newState);
  }
}

void MultiHwSwitchHandler::onHwInitialized(HwSwitchCallback* callback) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->onHwInitialized(callback);
  }
}

void MultiHwSwitchHandler::onInitialConfigApplied(HwSwitchCallback* sw) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->onInitialConfigApplied(sw);
  }
}

void MultiHwSwitchHandler::platformStop() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->platformStop();
  }
}

const AgentConfig* MultiHwSwitchHandler::config() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->config();
}

const AgentConfig* MultiHwSwitchHandler::reloadConfig() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->reloadConfig();
}

std::map<PortID, FabricEndpoint> MultiHwSwitchHandler::getFabricReachability() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getFabricReachability();
}

FabricReachabilityStats MultiHwSwitchHandler::getFabricReachabilityStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getFabricReachabilityStats();
}

std::vector<PortID> MultiHwSwitchHandler::getSwitchReachability(
    SwitchID switchId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSwitchReachability(switchId);
}

std::string MultiHwSwitchHandler::getDebugDump() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getDebugDump();
}

void MultiHwSwitchHandler::fetchL2Table(std::vector<L2EntryThrift>* l2Table) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->fetchL2Table(l2Table);
}

std::string MultiHwSwitchHandler::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->listObjects(types, cached);
}

bool MultiHwSwitchHandler::needL2EntryForNeighbor() {
  for (auto& entry : hwSwitchSyncers_) {
    if (entry.second->needL2EntryForNeighbor()) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<TxPacket> MultiHwSwitchHandler::allocatePacket(uint32_t size) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->allocatePacket(size);
}

bool MultiHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketOutOfPortAsync(
      std::move(pkt), portID, queue);
}

bool MultiHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedSync(
      std::move(pkt));
}

bool MultiHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedAsync(
      std::move(pkt));
}

std::optional<uint32_t> MultiHwSwitchHandler::getHwLogicalPortId(
    PortID portID) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getHwLogicalPortId(portID);
}

std::map<SwitchID, HwSwitchHandler*>
MultiHwSwitchHandler::getHwSwitchHandlers() {
  std::map<SwitchID, HwSwitchHandler*> handlers;
  for (const auto& [switchId, syncer] : hwSwitchSyncers_) {
    auto handler = static_cast<HwSwitchHandler*>(syncer.get());
    handlers.emplace(switchId, handler);
  }
  return handlers;
}

multiswitch::StateOperDelta MultiHwSwitchHandler::getNextStateOperDelta(
    int64_t switchId) {
  if (!isRunning()) {
    throw FbossError("multi hw switch syncer not started");
  }
  auto iter = hwSwitchSyncers_.find(SwitchID(switchId));
  CHECK(iter != hwSwitchSyncers_.end());
  return iter->second->getNextStateOperDelta();
}

void MultiHwSwitchHandler::cancelOperDeltaRequest(int64_t switchId) {
  if (!isRunning()) {
    throw FbossError("multi hw switch syncer not started");
  }
  auto iter = hwSwitchSyncers_.find(SwitchID(switchId));
  CHECK(iter != hwSwitchSyncers_.end());
  return iter->second->cancelOperDeltaRequest();
}

} // namespace facebook::fboss
