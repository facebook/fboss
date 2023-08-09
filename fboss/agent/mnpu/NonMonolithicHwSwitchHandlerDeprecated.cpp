// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandlerDeprecated.h"

#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

NonMonolithicHwSwitchHandlerDeprecated::
    NonMonolithicHwSwitchHandlerDeprecated() {}

void NonMonolithicHwSwitchHandlerDeprecated::exitFatal() const {
  // TODO: implement this
}

std::unique_ptr<TxPacket>
NonMonolithicHwSwitchHandlerDeprecated::allocatePacket(
    uint32_t /*size*/) const {
  // TODO: implement this
  return nullptr;
}

bool NonMonolithicHwSwitchHandlerDeprecated::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> /*pkt*/,
    PortID /*portID*/,
    std::optional<uint8_t> /*queue*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandlerDeprecated::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandlerDeprecated::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> /*pkt*/) noexcept {
  // TODO: implement this
  return true;
}

bool NonMonolithicHwSwitchHandlerDeprecated::isValidStateUpdate(
    const StateDelta& /*delta*/) const {
  // TODO: implement this
  return true;
}

void NonMonolithicHwSwitchHandlerDeprecated::unregisterCallbacks() {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandlerDeprecated::gracefulExit(
    state::WarmbootState& /*thriftSwitchState*/) {
  // TODO: implement this
}

bool NonMonolithicHwSwitchHandlerDeprecated::getAndClearNeighborHit(
    RouterID /*vrf*/,
    folly::IPAddress& /*ip*/) {
  return true; // TODO: implement this
}

folly::dynamic NonMonolithicHwSwitchHandlerDeprecated::toFollyDynamic() const {
  // TODO: implement this
  return folly::dynamic::object;
}

std::optional<uint32_t>
NonMonolithicHwSwitchHandlerDeprecated::getHwLogicalPortId(
    PortID /*portID*/) const {
  // TODO: implement this
  return std::nullopt;
}

void NonMonolithicHwSwitchHandlerDeprecated::initPlatformData() {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandlerDeprecated::onHwInitialized(
    HwSwitchCallback* /*callback*/) {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandlerDeprecated::onInitialConfigApplied(
    HwSwitchCallback* /*callback*/) {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandlerDeprecated::platformStop() {
  // TODO: implement this
}

const AgentConfig* NonMonolithicHwSwitchHandlerDeprecated::config() {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

const AgentConfig* NonMonolithicHwSwitchHandlerDeprecated::reloadConfig() {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

bool NonMonolithicHwSwitchHandlerDeprecated::transactionsSupported() const {
  // TODO: implement this
  return false;
}

folly::F14FastMap<std::string, HwPortStats>
NonMonolithicHwSwitchHandlerDeprecated::getPortStats() const {
  // TODO: implement this
  return {};
}

std::map<std::string, HwSysPortStats>
NonMonolithicHwSwitchHandlerDeprecated::getSysPortStats() const {
  // TODO: implement this
  return {};
}

void NonMonolithicHwSwitchHandlerDeprecated::updateStats(
    SwitchStats* /*switchStats*/) {
  // TODO: implement this
}

std::map<PortID, phy::PhyInfo>
NonMonolithicHwSwitchHandlerDeprecated::updateAllPhyInfo() {
  // TODO: implement this
  return {};
}

uint64_t NonMonolithicHwSwitchHandlerDeprecated::getDeviceWatermarkBytes()
    const {
  // TODO: implement this
  return 0;
}

HwSwitchFb303Stats* NonMonolithicHwSwitchHandlerDeprecated::getSwitchStats()
    const {
  // TODO: implement this
  // @lint-ignore CLANGTIDY
  return nullptr;
}

void NonMonolithicHwSwitchHandlerDeprecated::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& /*ports*/) {
  // TODO: implement this
}

std::vector<phy::PrbsLaneStats>
NonMonolithicHwSwitchHandlerDeprecated::getPortAsicPrbsStats(
    int32_t /*portId*/) {
  // TODO: implement this
  return {};
}

void NonMonolithicHwSwitchHandlerDeprecated::clearPortAsicPrbsStats(
    int32_t /*portId*/) {
  // TODO: implement this
}

std::vector<prbs::PrbsPolynomial>
NonMonolithicHwSwitchHandlerDeprecated::getPortPrbsPolynomials(
    int32_t /*portId*/) {
  // TODO: implement this
  return {};
}

prbs::InterfacePrbsState
NonMonolithicHwSwitchHandlerDeprecated::getPortPrbsState(PortID /* portId */) {
  // TODO: implement this
  return prbs::InterfacePrbsState{};
}

void NonMonolithicHwSwitchHandlerDeprecated::switchRunStateChanged(
    SwitchRunState /*newState*/) {
  // TODO: implement this
}

std::shared_ptr<SwitchState>
NonMonolithicHwSwitchHandlerDeprecated::stateChanged(
    const StateDelta& /*delta*/,
    bool /*transaction*/) {
  // TODO: implement this
  return nullptr;
}

CpuPortStats NonMonolithicHwSwitchHandlerDeprecated::getCpuPortStats() const {
  throw FbossError("getCpuPortStats not implemented");
}

std::map<PortID, FabricEndpoint>
NonMonolithicHwSwitchHandlerDeprecated::getFabricReachability() const {
  throw FbossError("getFabricReachability not implemented");
}

std::vector<PortID>
NonMonolithicHwSwitchHandlerDeprecated::getSwitchReachability(
    SwitchID /*switchId*/) const {
  throw FbossError("getSwitchReachability not implemented");
}

std::string NonMonolithicHwSwitchHandlerDeprecated::getDebugDump() const {
  throw FbossError("getDebugDump not implemented");
}

void NonMonolithicHwSwitchHandlerDeprecated::fetchL2Table(
    std::vector<L2EntryThrift>* /*l2Table*/) const {
  throw FbossError("fetchL2Table not implemented");
}

std::string NonMonolithicHwSwitchHandlerDeprecated::listObjects(
    const std::vector<HwObjectType>& /*types*/,
    bool /*cached*/) const {
  throw FbossError("listObjects not implemented");
}

FabricReachabilityStats
NonMonolithicHwSwitchHandlerDeprecated::getFabricReachabilityStats() const {
  throw FbossError("getFabricReachabilityStats not implemented");
}

bool NonMonolithicHwSwitchHandlerDeprecated::needL2EntryForNeighbor() const {
  throw FbossError("listObjects not implemented");
}

fsdb::OperDelta NonMonolithicHwSwitchHandlerDeprecated::stateChanged(
    const fsdb::OperDelta& /*delta*/,
    bool /*transaction*/) {
  throw FbossError("stateChanged not implemented");
}

} // namespace facebook::fboss
