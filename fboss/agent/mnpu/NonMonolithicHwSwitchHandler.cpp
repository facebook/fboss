// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"

#include "fboss/agent/TxPacket.h"

namespace facebook::fboss {

NonMonolithicHwSwitchHandler::NonMonolithicHwSwitchHandler() {}

HwInitResult NonMonolithicHwSwitchHandler::initHw(
    HwSwitchCallback* /*callback*/,
    bool /*failHwCallsOnWarmboot*/) {
  // TODO: implement this
  return HwInitResult{};
}

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

bool NonMonolithicHwSwitchHandler::isValidStateUpdate(
    const StateDelta& /*delta*/) const {
  // TODO: implement this
  return true;
}

void NonMonolithicHwSwitchHandler::unregisterCallbacks() {
  // TODO: implement this
}

void NonMonolithicHwSwitchHandler::gracefulExit(
    folly::dynamic& /*follySwitchState*/,
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

void NonMonolithicHwSwitchHandler::initPlatformData() {
  // TODO: implement this
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

std::vector<phy::PrbsLaneStats>
NonMonolithicHwSwitchHandler::getPortGearboxPrbsStats(
    int32_t /*portId*/,
    phy::Side /*side*/) {
  // TODO: implement this
  return {};
}

void NonMonolithicHwSwitchHandler::clearPortGearboxPrbsStats(
    int32_t /*portId*/,
    phy::Side /*side*/) {
  // TODO: implement this
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
} // namespace facebook::fboss
