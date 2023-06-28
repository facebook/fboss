// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicHwSwitchHandler.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

MonolinithicHwSwitchHandler::MonolinithicHwSwitchHandler(
    PlatformInitFn initPlatformFn)
    : initPlatformFn_(std::move(initPlatformFn)) {}

void MonolinithicHwSwitchHandler::initPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t features) {
  platform_ = initPlatformFn_(std::move(config), features);
  hw_ = platform_->getHwSwitch();
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

} // namespace facebook::fboss
