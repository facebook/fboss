// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/agent/MultiSwitchPacketStreamMap.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

NonMonolithicHwSwitchHandler::NonMonolithicHwSwitchHandler(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info,
    SwSwitch* sw)
    : HwSwitchHandler(switchId, info), sw_(sw) {}

void NonMonolithicHwSwitchHandler::exitFatal() const {
  // TODO: implement this
}

std::unique_ptr<TxPacket> NonMonolithicHwSwitchHandler::allocatePacket(
    uint32_t size) const {
  return TxPacket::allocateTxPacket(size);
}

bool NonMonolithicHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt), portID, queue);
}

bool NonMonolithicHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt));
}

bool NonMonolithicHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt));
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
  // TODO: query hwswitch and return logical port id
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

bool NonMonolithicHwSwitchHandler::transactionsSupported(
    std::optional<cfg::SdkVersion> sdkVersion) const {
  if (sdkVersion.has_value() && sdkVersion.value().saiSdk().has_value()) {
    return true;
  }
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

// not used in split
void NonMonolithicHwSwitchHandler::updateStats() {}

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

bool NonMonolithicHwSwitchHandler::needL2EntryForNeighbor(
    const cfg::SwitchConfig* config) const {
  // if config is not present, fall back to true
  // if sdk version is not set (for test configs), fall back to true
  // if sai, return true else false
  // falling back to true as assumption is split mode is for SAI alone
  return !config || !config->sdkVersion().has_value() ||
      config->sdkVersion()->saiSdk().has_value();
}

bool NonMonolithicHwSwitchHandler::sendPacketOutViaThriftStream(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortID> portID,
    std::optional<uint8_t> queue) {
  sw_->sendPacketOutViaThriftStream(
      std::move(pkt), getSwitchId(), portID, queue);
  return true;
}

bool NonMonolithicHwSwitchHandler::isOperSyncState(
    HwSwitchOperDeltaSyncState state) const {
  return operDeltaSyncState_ == state;
}

std::pair<fsdb::OperDelta, HwSwitchStateUpdateStatus>
NonMonolithicHwSwitchHandler::stateChanged(
    const fsdb::OperDelta& delta,
    bool transaction) {
  multiswitch::StateOperDelta stateDelta;
  stateDelta.operDelta() = delta;
  stateDelta.transaction() = transaction;
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    if (isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED)) {
      // return incoming delta to indicate that none of the changes were applied
      return {
          delta, HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
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
    stateUpdateCV_.wait(lk, [this] {
      return (
          ackReceived_ ||
          isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED));
    });
    if (isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED)) {
      // return incoming delta to indicate that none of the changes were applied
      return {
          delta, HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
    }
  }
  // received ack. return result from HwSwitch
  // TODO - handle failures and do rollback on succeeded switches
  return {
      *prevOperDeltaResult_->operDelta(),
      prevOperDeltaResult_->operDelta()->changes()->empty()
          ? HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_SUCCEEDED
          : HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_FAILED};
}

multiswitch::StateOperDelta NonMonolithicHwSwitchHandler::getNextStateOperDelta(
    std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
    bool initialSync) {
  // check whether it is a new connection.
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    if ((isOperSyncState(HwSwitchOperDeltaSyncState::DISCONNECTED) ||
         isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED)) &&
        initialSync) {
      setOperSyncState(HwSwitchOperDeltaSyncState::WAITING_INITIAL_SYNC);
    } else {
      // For existing connections, we treat a new get request
      // as an ack to pending state update.
      if (isOperSyncState(HwSwitchOperDeltaSyncState::INITIAL_OPER_SENT)) {
        setOperSyncState(HwSwitchOperDeltaSyncState::OPER_SYNCED);
      }
      ackReceived_ = true;
      prevOperDeltaResult_ = prevOperResult.get();
    }
  }
  stateUpdateCV_.notify_one();

  // wait for new delta to be available or for cancellation.
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    stateUpdateCV_.wait(lk, [this] {
      return (
          deltaReady_ ||
          isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED));
    });
    if (isOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED)) {
      // return empty delta to HwSwitch incase of cancellation
      return multiswitch::StateOperDelta();
    }
    deltaReady_ = false;
    if (isOperSyncState(HwSwitchOperDeltaSyncState::WAITING_INITIAL_SYNC)) {
      setOperSyncState(HwSwitchOperDeltaSyncState::INITIAL_OPER_SENT);
    }
    return *nextOperDelta_;
  }
}

void NonMonolithicHwSwitchHandler::notifyHwSwitchDisconnected() {
  // cancel any pending operations.
  cancelOperDeltaSync();
}

void NonMonolithicHwSwitchHandler::cancelOperDeltaSync() {
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    setOperSyncState(HwSwitchOperDeltaSyncState::CANCELLED);
    nextOperDelta_ = nullptr;
  }
  stateUpdateCV_.notify_all();
}

NonMonolithicHwSwitchHandler::~NonMonolithicHwSwitchHandler() {
  cancelOperDeltaSync();
}

HwSwitchOperDeltaSyncState
NonMonolithicHwSwitchHandler::getHwSwitchOperDeltaSyncState() {
  std::unique_lock<std::mutex> lk(stateUpdateMutex_);
  return operDeltaSyncState_;
}

} // namespace facebook::fboss
