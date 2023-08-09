// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiHwSwitchHandlerWIP.h"
#include "fboss/agent/HwSwitchHandlerDeprecated.h"
#include "fboss/agent/HwSwitchHandlerWIP.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

MultiHwSwitchHandlerWIP::MultiHwSwitchHandlerWIP(
    const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn) {
  for (auto entry : switchInfoMap) {
    hwSwitchSyncers_.emplace(
        SwitchID(entry.first),
        hwSwitchHandlerInitFn(SwitchID(entry.first), entry.second));
  }
}

MultiHwSwitchHandlerWIP::~MultiHwSwitchHandlerWIP() {
  stop();
}

void MultiHwSwitchHandlerWIP::start() {
  if (!stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->start();
  }
  stopped_.store(false);
}

void MultiHwSwitchHandlerWIP::stop() {
  if (stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second.reset();
  }
  stopped_.store(true);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandlerWIP::stateChanged(
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

folly::Future<std::shared_ptr<SwitchState>>
MultiHwSwitchHandlerWIP::stateChanged(
    SwitchID switchId,
    const HwSwitchStateUpdate& update) {
  auto iter = hwSwitchSyncers_.find(switchId);
  if (iter == hwSwitchSyncers_.end()) {
    throw FbossError("hw switch syncer for switch id ", switchId, " not found");
  }
  return iter->second->stateChanged(update);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandlerWIP::getStateUpdateResult(
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

HwSwitchHandlerWIP* MultiHwSwitchHandlerWIP::getHwSwitchHandler(
    SwitchID switchId) {
  auto handler = hwSwitchSyncers_.find(switchId);
  if (handler == hwSwitchSyncers_.end()) {
    throw FbossError(
        "No hw switch handler for switch id: ", switchId, " found!");
  }
  return handler->second.get();
}

void MultiHwSwitchHandlerWIP::unregisterCallbacks() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->unregisterCallbacks();
  }
}

void MultiHwSwitchHandlerWIP::exitFatal() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->exitFatal();
  }
}

bool MultiHwSwitchHandlerWIP::isValidStateUpdate(const StateDelta& delta) {
  for (auto& entry : hwSwitchSyncers_) {
    if (!entry.second->isValidStateUpdate(delta)) {
      return false;
    }
  }
  return true;
}

void MultiHwSwitchHandlerWIP::gracefulExit(
    state::WarmbootState& thriftSwitchState) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->gracefulExit(thriftSwitchState);
  }
}

bool MultiHwSwitchHandlerWIP::getAndClearNeighborHit(
    RouterID vrf,
    folly::IPAddress& ip) {
  for (auto& entry : hwSwitchSyncers_) {
    if (entry.second->getAndClearNeighborHit(vrf, ip)) {
      return true;
    }
  }
  return false;
}

folly::dynamic MultiHwSwitchHandlerWIP::toFollyDynamic() {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->toFollyDynamic();
}

const PlatformData& MultiHwSwitchHandlerWIP::getPlatformData() const {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPlatformData();
}

bool MultiHwSwitchHandlerWIP::transactionsSupported() {
  for (auto& entry : hwSwitchSyncers_) {
    if (!entry.second->transactionsSupported()) {
      return false;
    }
  }
  return true;
}

HwSwitchFb303Stats* MultiHwSwitchHandlerWIP::getSwitchStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSwitchStats();
}

folly::F14FastMap<std::string, HwPortStats>
MultiHwSwitchHandlerWIP::getPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortStats();
}

CpuPortStats MultiHwSwitchHandlerWIP::getCpuPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getCpuPortStats();
}

std::map<std::string, HwSysPortStats>
MultiHwSwitchHandlerWIP::getSysPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSysPortStats();
}

void MultiHwSwitchHandlerWIP::updateStats(SwitchStats* switchStats) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->updateStats(switchStats);
}

std::map<PortID, phy::PhyInfo> MultiHwSwitchHandlerWIP::updateAllPhyInfo() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->updateAllPhyInfo();
}

uint64_t MultiHwSwitchHandlerWIP::getDeviceWatermarkBytes() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getDeviceWatermarkBytes();
}

void MultiHwSwitchHandlerWIP::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats> MultiHwSwitchHandlerWIP::getPortAsicPrbsStats(
    int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortAsicPrbsStats(portId);
}

void MultiHwSwitchHandlerWIP::clearPortAsicPrbsStats(int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial>
MultiHwSwitchHandlerWIP::getPortPrbsPolynomials(int32_t portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState MultiHwSwitchHandlerWIP::getPortPrbsState(
    PortID portId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getPortPrbsState(portId);
}

void MultiHwSwitchHandlerWIP::switchRunStateChanged(SwitchRunState newState) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->switchRunStateChanged(newState);
  }
}

void MultiHwSwitchHandlerWIP::onHwInitialized(HwSwitchCallback* callback) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->onHwInitialized(callback);
  }
}

void MultiHwSwitchHandlerWIP::onInitialConfigApplied(HwSwitchCallback* sw) {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->onInitialConfigApplied(sw);
  }
}

void MultiHwSwitchHandlerWIP::platformStop() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->platformStop();
  }
}

const AgentConfig* MultiHwSwitchHandlerWIP::config() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->config();
}

const AgentConfig* MultiHwSwitchHandlerWIP::reloadConfig() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->reloadConfig();
}

std::map<PortID, FabricEndpoint>
MultiHwSwitchHandlerWIP::getFabricReachability() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getFabricReachability();
}

FabricReachabilityStats MultiHwSwitchHandlerWIP::getFabricReachabilityStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getFabricReachabilityStats();
}

std::vector<PortID> MultiHwSwitchHandlerWIP::getSwitchReachability(
    SwitchID switchId) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSwitchReachability(switchId);
}

std::string MultiHwSwitchHandlerWIP::getDebugDump() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getDebugDump();
}

void MultiHwSwitchHandlerWIP::fetchL2Table(
    std::vector<L2EntryThrift>* l2Table) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->fetchL2Table(l2Table);
}

std::string MultiHwSwitchHandlerWIP::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->listObjects(types, cached);
}

bool MultiHwSwitchHandlerWIP::needL2EntryForNeighbor() {
  for (auto& entry : hwSwitchSyncers_) {
    if (entry.second->needL2EntryForNeighbor()) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<TxPacket> MultiHwSwitchHandlerWIP::allocatePacket(
    uint32_t size) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->allocatePacket(size);
}

bool MultiHwSwitchHandlerWIP::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketOutOfPortAsync(
      std::move(pkt), portID, queue);
}

bool MultiHwSwitchHandlerWIP::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedSync(
      std::move(pkt));
}

bool MultiHwSwitchHandlerWIP::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  // Not supported with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedAsync(
      std::move(pkt));
}

std::optional<uint32_t> MultiHwSwitchHandlerWIP::getHwLogicalPortId(
    PortID portID) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getHwLogicalPortId(portID);
}

std::map<SwitchID, HwSwitchHandlerWIP*>
MultiHwSwitchHandlerWIP::getHwSwitchHandlers() {
  std::map<SwitchID, HwSwitchHandlerWIP*> handlers;
  for (const auto& [switchId, syncer] : hwSwitchSyncers_) {
    auto handler = static_cast<HwSwitchHandlerWIP*>(syncer.get());
    handlers.emplace(switchId, handler);
  }
  return handlers;
}

} // namespace facebook::fboss
