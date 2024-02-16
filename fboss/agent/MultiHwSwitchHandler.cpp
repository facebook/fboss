// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiHwSwitchHandler.h"
#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

namespace {
auto makeHwSwitchSyncers(
    SwSwitch* sw,
    const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn) {
  std::map<SwitchID, std::unique_ptr<HwSwitchHandler>> syncers;
  for (auto entry : switchInfoMap) {
    syncers.emplace(
        SwitchID(entry.first),
        hwSwitchHandlerInitFn(SwitchID(entry.first), entry.second, sw));
  }
  return syncers;
}
} // namespace

MultiHwSwitchHandler::MultiHwSwitchHandler(
    const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn,
    SwSwitch* sw,
    std::optional<cfg::SdkVersion> sdkVersion)
    : sw_(sw),
      hwSwitchSyncers_(
          makeHwSwitchSyncers(sw, switchInfoMap, hwSwitchHandlerInitFn)),
      transactionsSupported_(transactionsSupported(sdkVersion)) {}

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
  // Cancel any pending waits for HwSwitch connect calls
  connectionStatusTable_.cancelWait();
  // set stop flag so that there are no more accesses to syncers
  stopped_.store(true);
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->stop();
  }
}

std::shared_ptr<SwitchState> MultiHwSwitchHandler::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  std::map<SwitchID, const StateDelta&> deltas;
  std::shared_ptr<SwitchState> newState{nullptr};
  bool updateFailed{false};
  if (stopped_.load()) {
    throw FbossError("multi hw switch syncer not started");
  }
  for (const auto& entry : hwSwitchSyncers_) {
    auto switchId = entry.first;
    deltas.emplace(switchId, delta);
  }
  auto results = stateChanged(deltas, transaction);
  for (const auto& result : results) {
    auto status = result.second.second;
    if (status == HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_SUCCEEDED) {
      newState = result.second.first;
    } else if (
        status == HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_FAILED) {
      updateFailed = true;
    }
  }
  if (updateFailed) {
    if (transactionsSupported()) {
      return rollbackStateChange(results, delta.oldState(), transaction);
    } else {
      /*
       * For deployments where we don't support transactions - e.g. legacy
       * BCMSwitch implementations. We never expect more than one
       * HwSwitch/HwSwitchSyncer
       */
      CHECK_EQ(hwSwitchSyncers_.size(), 1);
      return results[SwitchID(0)].first;
    }
  }
  /* none of the switches were updated */
  if (!newState) {
    return delta.oldState();
  }
  return newState;
}

std::shared_ptr<SwitchState> MultiHwSwitchHandler::rollbackStateChange(
    const std::map<SwitchID, HwSwitchStateUpdateResult>& updateResults,
    std::shared_ptr<SwitchState> desiredState,
    bool transaction) {
  std::map<SwitchID, const StateDelta&> switchIdAndDeltas;
  std::shared_ptr<SwitchState> newState{nullptr};
  std::set<std::unique_ptr<StateDelta>> deltas;
  int index{0};
  for (const auto& entry : updateResults) {
    auto switchId = entry.first;
    auto status = entry.second.second;
    auto currentState = entry.second.first;
    if (status != HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED) {
      auto delta = std::make_unique<StateDelta>(currentState, desiredState);
      switchIdAndDeltas.emplace(switchId, *delta);
      deltas.insert(std::move(delta));
    }
    index++;
  }
  auto results = stateChanged(switchIdAndDeltas, transaction);
  for (const auto& result : results) {
    if (result.second.second ==
        HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_FAILED) {
      throw FbossError(
          "Failed to rollback switch state on switch id ", result.first);
    }
  }
  return desiredState;
}

std::map<SwitchID, HwSwitchStateUpdateResult>
MultiHwSwitchHandler::stateChanged(
    const std::map<SwitchID, const StateDelta&>& deltas,
    bool transaction) {
  std::vector<SwitchID> switchIds;
  std::vector<folly::Future<HwSwitchStateUpdateResult>> futures;
  for (const auto& entry : deltas) {
    switchIds.push_back(entry.first);
    auto update = HwSwitchStateUpdate(entry.second, transaction);
    futures.emplace_back(stateChanged(entry.first, update));
  }
  return getStateUpdateResult(switchIds, futures);
}

folly::Future<HwSwitchStateUpdateResult> MultiHwSwitchHandler::stateChanged(
    SwitchID switchId,
    const HwSwitchStateUpdate& update) {
  auto iter = hwSwitchSyncers_.find(switchId);
  if (iter == hwSwitchSyncers_.end()) {
    throw FbossError("hw switch syncer for switch id ", switchId, " not found");
  }
  return iter->second->stateChanged(update);
}

std::map<SwitchID, HwSwitchStateUpdateResult>
MultiHwSwitchHandler::getStateUpdateResult(
    const std::vector<SwitchID>& switchIds,
    std::vector<folly::Future<HwSwitchStateUpdateResult>>& futures) const {
  std::map<SwitchID, HwSwitchStateUpdateResult> hwUpdateResults;

  auto results = folly::collectAll(futures).wait();
  auto index{0};
  std::shared_ptr<SwitchState> newState;
  for (const auto& result : results.value()) {
    CHECK_LT(index, switchIds.size());
    auto switchId = switchIds[index];
    if (result.hasException()) {
      XLOG(ERR) << "Failed to get state update result for switch id "
                << switchId << ":" << result.exception().what();
      result.exception().throw_exception();
    }
    hwUpdateResults.emplace(switchId, result.value());
    index++;
  }
  return hwUpdateResults;
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

void MultiHwSwitchHandler::gracefulExit() {
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->gracefulExit();
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

bool MultiHwSwitchHandler::transactionsSupported() const {
  return transactionsSupported_;
}

bool MultiHwSwitchHandler::transactionsSupported(
    std::optional<cfg::SdkVersion> sdkVersion) const {
  for (auto& entry : hwSwitchSyncers_) {
    if (!entry.second->transactionsSupported(sdkVersion)) {
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

CpuPortStats MultiHwSwitchHandler::getCpuPortStats(bool getIncrement) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getCpuPortStats(getIncrement);
}

std::map<std::string, HwSysPortStats> MultiHwSwitchHandler::getSysPortStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSysPortStats();
}

HwSwitchDropStats MultiHwSwitchHandler::getSwitchDropStats() const {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getSwitchDropStats();
}

void MultiHwSwitchHandler::updateStats() {
  return hwSwitchSyncers_.begin()->second->updateStats();
}

void MultiHwSwitchHandler::updateAllPhyInfo() {
  hwSwitchSyncers_.begin()->second->updateAllPhyInfo();
}

std::map<PortID, phy::PhyInfo> MultiHwSwitchHandler::getAllPhyInfo() const {
  return hwSwitchSyncers_.begin()->second->getAllPhyInfo();
}

uint64_t MultiHwSwitchHandler::getDeviceWatermarkBytes() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getDeviceWatermarkBytes();
}

HwFlowletStats MultiHwSwitchHandler::getHwFlowletStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getHwFlowletStats();
}

void MultiHwSwitchHandler::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->clearPortStats(ports);
}

std::vector<phy::PrbsLaneStats> MultiHwSwitchHandler::getPortAsicPrbsStats(
    PortID portId) {
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

std::vector<EcmpDetails> MultiHwSwitchHandler::getAllEcmpDetails() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getAllEcmpDetails();
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

std::map<PortID, FabricEndpoint> MultiHwSwitchHandler::getFabricConnectivity() {
  // TODO - retire this api after migrating clients to HwAgent api
  return hwSwitchSyncers_.begin()->second->getFabricConnectivity();
}

FabricReachabilityStats MultiHwSwitchHandler::getFabricReachabilityStats() {
  // TODO - support with multiple switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->getFabricReachabilityStats();
}

std::vector<PortID> MultiHwSwitchHandler::getSwitchReachability(
    SwitchID switchId) {
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

bool MultiHwSwitchHandler::needL2EntryForNeighbor(
    const cfg::SwitchConfig* config) const {
  for (auto& entry : hwSwitchSyncers_) {
    if (entry.second->needL2EntryForNeighbor(config)) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<TxPacket> MultiHwSwitchHandler::allocatePacket(uint32_t size) {
  // TODO - support with multiple switches
  CHECK_GE(hwSwitchSyncers_.size(), 1);
  return hwSwitchSyncers_.begin()->second->allocatePacket(size);
}

bool MultiHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  auto switchId = sw_->getScopeResolver()->scope(portID).switchId();
  auto iter = hwSwitchSyncers_.find(switchId);
  if (iter == hwSwitchSyncers_.end()) {
    XLOG(ERR) << " hw switch syncer for switch id " << switchId << " not found";
    return false;
  }
  return iter->second->sendPacketOutOfPortAsync(std::move(pkt), portID, queue);
}

bool MultiHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  CHECK_GE(hwSwitchSyncers_.size(), 1);
  // use first available switch to send pkt
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedSync(
      std::move(pkt));
}

bool MultiHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  CHECK_GE(hwSwitchSyncers_.size(), 1);
  // use first available switch to send pkt
  return hwSwitchSyncers_.begin()->second->sendPacketSwitchedAsync(
      std::move(pkt));
}

std::optional<uint32_t> MultiHwSwitchHandler::getHwLogicalPortId(
    PortID portID) {
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
    int64_t switchId,
    std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
    int64_t lastUpdateSeqNum) {
  if (!isRunning()) {
    throw FbossError("multi hw switch syncer not started");
  }
  auto iter = hwSwitchSyncers_.find(SwitchID(switchId));
  CHECK(iter != hwSwitchSyncers_.end());
  return iter->second->getNextStateOperDelta(
      std::move(prevOperResult), lastUpdateSeqNum);
}

void MultiHwSwitchHandler::notifyHwSwitchGracefulExit(int64_t switchId) {
  notifyHwSwitchDisconnected(switchId, true);
}

void MultiHwSwitchHandler::notifyHwSwitchDisconnected(
    int64_t switchId,
    bool gracefulExit) {
  if (!isRunning()) {
    throw FbossError("multi hw switch syncer not started");
  }
  auto iter = hwSwitchSyncers_.find(SwitchID(switchId));
  CHECK(iter != hwSwitchSyncers_.end());

  if (connectionStatusTable_.disconnected(SwitchID(switchId))) {
    // cancel any pending long poll request
    iter->second->notifyHwSwitchDisconnected();
    if (!gracefulExit) {
      sw_->setPortsDownForSwitch(SwitchID(switchId));
    }
  }
}

bool MultiHwSwitchHandler::waitUntilHwSwitchConnected() {
  return connectionStatusTable_.waitUntilHwSwitchConnected();
}

std::map<int32_t, SwitchRunState> MultiHwSwitchHandler::getHwSwitchRunStates() {
  std::map<int32_t, SwitchRunState> runStates;
  if (!isRunning()) {
    throw FbossError("multi hw switch handler not started");
  }
  for (const auto& [switchId, syncer] : hwSwitchSyncers_) {
    runStates[static_cast<int32_t>(switchId)] = syncer->getHwSwitchRunState();
  }
  return runStates;
}

} // namespace facebook::fboss
