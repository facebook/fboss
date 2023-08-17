// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

NonMonolithicHwSwitchHandler::NonMonolithicHwSwitchHandler(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)
    : HwSwitchHandler(switchId, info) {}

void NonMonolithicHwSwitchHandler::exitFatal() const {
  // TODO: implement this
}

std::unique_ptr<TxPacket> NonMonolithicHwSwitchHandler::allocatePacket(
    uint32_t /*size*/) const {
  // TODO: implement this
  return nullptr;
}

bool NonMonolithicHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> /*pkt*/,
    PortID /*portID*/,
    std::optional<uint8_t> /*queue*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandler::isValidStateUpdate(
    const StateDelta& /*delta*/) const {
  // TODO: implement this
  return true;
}

void NonMonolithicHwSwitchHandler::unregisterCallbacks() {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandler::gracefulExit(
    state::WarmbootState& /*thriftSwitchState*/) {
  // TODO: implement this
}

bool NonMonolithicHwSwitchHandler::getAndClearNeighborHit(
    RouterID /*vrf*/,
    folly::IPAddress& /*ip*/) {
  return true; // TODO: implement this
}

folly::dynamic NonMonolithicHwSwitchHandler::toFollyDynamic() const {
  // TODO: implement this
  return folly::dynamic::object;
}

std::optional<uint32_t> NonMonolithicHwSwitchHandler::getHwLogicalPortId(
    PortID /*portID*/) const {
  // TODO: implement this
  return std::nullopt;
}

void NonMonolithicHwSwitchHandler::onHwInitialized(
    HwSwitchCallback* /*callback*/) {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandler::onInitialConfigApplied(
    HwSwitchCallback* /*callback*/) {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandler::platformStop() {
  // TODO: implement this
}

const AgentConfig* NonMonolithicHwSwitchHandler::config() {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

const AgentConfig* NonMonolithicHwSwitchHandler::reloadConfig() {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

bool NonMonolithicHwSwitchHandler::transactionsSupported() const {
  // TODO: implement this
  return false;
}

folly::F14FastMap<std::string, HwPortStats>
NonMonolithicHwSwitchHandler::getPortStats() const {
  // TODO: implement this
  return {};
}

std::map<std::string, HwSysPortStats>
NonMonolithicHwSwitchHandler::getSysPortStats() const {
  // TODO: implement this
  return {};
}

void NonMonolithicHwSwitchHandler::updateStats(SwitchStats* /*switchStats*/) {
  // TODO: implement this
}

std::map<PortID, phy::PhyInfo>
NonMonolithicHwSwitchHandler::updateAllPhyInfo() {
  // TODO: implement this
  return {};
}

uint64_t NonMonolithicHwSwitchHandler::getDeviceWatermarkBytes() const {
  // TODO: implement this
  return 0;
}

HwSwitchFb303Stats* NonMonolithicHwSwitchHandler::getSwitchStats() const {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

void NonMonolithicHwSwitchHandler::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& /*ports*/) {
  // TODO: implement this
}

std::vector<phy::PrbsLaneStats>
NonMonolithicHwSwitchHandler::getPortAsicPrbsStats(int32_t /*portId*/) {
  // TODO: implement this
  return {};
}

void NonMonolithicHwSwitchHandler::clearPortAsicPrbsStats(int32_t /*portId*/) {
  // TODO: implement this
}

std::vector<prbs::PrbsPolynomial>
NonMonolithicHwSwitchHandler::getPortPrbsPolynomials(int32_t /*portId*/) {
  // TODO: implement this
  return {};
}

prbs::InterfacePrbsState NonMonolithicHwSwitchHandler::getPortPrbsState(
    PortID /* portId */) {
  // TODO: implement this
  return prbs::InterfacePrbsState{};
}

void NonMonolithicHwSwitchHandler::switchRunStateChanged(
    SwitchRunState /*newState*/) {
  // TODO: implement this
}

std::shared_ptr<SwitchState> NonMonolithicHwSwitchHandler::stateChanged(
    const StateDelta& /*delta*/,
    bool /*transaction*/) {
  // TODO: implement this
  return nullptr;
}

CpuPortStats NonMonolithicHwSwitchHandler::getCpuPortStats() const {
  throw FbossError("getCpuPortStats not implemented");
}

std::map<PortID, FabricEndpoint>
NonMonolithicHwSwitchHandler::getFabricReachability() const {
  throw FbossError("getFabricReachability not implemented");
}

std::vector<PortID> NonMonolithicHwSwitchHandler::getSwitchReachability(
    SwitchID /*switchId*/) const {
  throw FbossError("getSwitchReachability not implemented");
}

std::string NonMonolithicHwSwitchHandler::getDebugDump() const {
  throw FbossError("getDebugDump not implemented");
}

void NonMonolithicHwSwitchHandler::fetchL2Table(
    std::vector<L2EntryThrift>* /*l2Table*/) const {
  throw FbossError("fetchL2Table not implemented");
}

std::string NonMonolithicHwSwitchHandler::listObjects(
    const std::vector<HwObjectType>& /*types*/,
    bool /*cached*/) const {
  throw FbossError("listObjects not implemented");
}

FabricReachabilityStats
NonMonolithicHwSwitchHandler::getFabricReachabilityStats() const {
  throw FbossError("getFabricReachabilityStats not implemented");
}

bool NonMonolithicHwSwitchHandler::needL2EntryForNeighbor() const {
  throw FbossError("listObjects not implemented");
}

fsdb::OperDelta NonMonolithicHwSwitchHandler::stateChanged(
    const fsdb::OperDelta& delta,
    bool transaction) {
  multiswitch::StateOperDelta stateDelta;
  stateDelta.operDelta() = delta;
  stateDelta.transaction() = transaction;
  // if HwSwitch is not connected, wait for connection
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    if (!connected_) {
      stateUpdateCV_.wait(lk, [this] { return connected_; });
    }
    nextOperDelta_ = &stateDelta;
    ackReceived_ = false;
    deltaReady_ = true;
  }
  // state update ready. notify waiting thread
  stateUpdateCV_.notify_one();
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    // wait for acknowledgement
    stateUpdateCV_.wait(
        lk, [this] { return ackReceived_ || deltaReadCancelled_; });
    if (deltaReadCancelled_) {
      // TODO - handle cancellation by HwSwitch
      throw FbossError("client cancelled delta read during update");
    }
  }
  // received ack. return empty delta to indicate success
  return fsdb::OperDelta();
}

multiswitch::StateOperDelta
NonMonolithicHwSwitchHandler::getNextStateOperDelta() {
  // check whether it is a new connection.
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    if (!connected_) {
      connected_ = true;
    } else {
      // For existing connections, we treat a new get request
      // as an ack to pending state update.
      ackReceived_ = true;
    }
  }
  stateUpdateCV_.notify_one();

  // wait for new delta to be available or for cancellation.
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    stateUpdateCV_.wait(
        lk, [this] { return deltaReady_ || deltaReadCancelled_; });
    if (deltaReadCancelled_) {
      // return empty delta to HwSwitch incase of cancellation
      return multiswitch::StateOperDelta();
    }
    deltaReady_ = false;
    return *nextOperDelta_;
  }
}

void NonMonolithicHwSwitchHandler::cancelOperDeltaRequest() {
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    connected_ = false;
    deltaReadCancelled_ = true;
  }
  stateUpdateCV_.notify_all();
}
} // namespace facebook::fboss
