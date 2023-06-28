#pragma once

#include "fboss/agent/HwSwitchHandler.h"

namespace facebook::fboss {

class NonMonolithicHwSwitchHandler : public HwSwitchHandler {
 public:
  NonMonolithicHwSwitchHandler();

  virtual ~NonMonolithicHwSwitchHandler() override = default;

  void initPlatform(
      std::unique_ptr<AgentConfig> /*config*/,
      uint32_t /*features*/) override {}

  HwInitResult initHw(HwSwitchCallback* callback, bool failHwCallsOnWarmboot)
      override;

  void exitFatal() const override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  void unregisterCallbacks() override;

  void gracefulExit(
      folly::dynamic& follySwitchState,
      state::WarmbootState& thriftSwitchState) override;

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  folly::dynamic toFollyDynamic() const override;

  std::optional<uint32_t> getHwLogicalPortId(PortID portID) const override;
};

} // namespace facebook::fboss
