// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwSwitchHandler.h"

namespace facebook::fboss {

class HwSwitch;
class Platform;
class TxPacket;

class MonolinithicHwSwitchHandler : public HwSwitchHandler {
 public:
  using PlatformInitFn = std::function<std::unique_ptr<Platform>(
      std::unique_ptr<AgentConfig>,
      uint32_t featuresDesired)>;

  /* TODO: remove this constructor */
  explicit MonolinithicHwSwitchHandler(std::unique_ptr<Platform> platform);

  explicit MonolinithicHwSwitchHandler(PlatformInitFn initPlatformFn);

  virtual ~MonolinithicHwSwitchHandler() override {}

  void initPlatform(std::unique_ptr<AgentConfig> config, uint32_t features)
      override;

  HwInitResult initHw(HwSwitchCallback* callback, bool failHwCallsOnWarmboot)
      override;

  void exitFatal() const override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  void unregisterCallbacks() override;

  void gracefulExit(
      folly::dynamic& follySwitchState,
      state::WarmbootState& thriftSwitchState) override;

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  folly::dynamic toFollyDynamic() const override;

  std::optional<uint32_t> getHwLogicalPortId(PortID portID) const override;

  void initPlatformData() override;

  folly::F14FastMap<std::string, HwPortStats> getPortStats() const override;

  std::map<std::string, HwSysPortStats> getSysPortStats() const override;

  void updateStats(SwitchStats* switchStats) override;

  std::map<PortID, phy::PhyInfo> updateAllPhyInfo() override;

  uint64_t getDeviceWatermarkBytes() const override;

  HwSwitchFb303Stats* getSwitchStats() const override;

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;

  std::vector<phy::PrbsLaneStats> getPortAsicPrbsStats(int32_t portId) override;

  void clearPortAsicPrbsStats(int32_t portId) override;

  std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials(
      int32_t portId) override;

  prbs::InterfacePrbsState getPortPrbsState(PortID portId) override;

  std::vector<phy::PrbsLaneStats> getPortGearboxPrbsStats(
      int32_t portId,
      phy::Side side) override;

  void clearPortGearboxPrbsStats(int32_t portId, phy::Side side) override;

  void switchRunStateChanged(SwitchRunState newState) override;

  // platform access apis
  void onHwInitialized(HwSwitchCallback* callback) override;

  void onInitialConfigApplied(HwSwitchCallback* sw) override;

  void platformStop() override;

  const AgentConfig* config() override;

  const AgentConfig* reloadConfig() override;

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction) override;

  bool transactionsSupported() const override;

  /* TODO: remove this method */
  HwSwitch* getHwSwitch() const {
    return hw_;
  }

  /* TODO: remove this method */
  Platform* getPlatform() const {
    return platform_.get();
  }

 private:
  PlatformInitFn initPlatformFn_;
  std::unique_ptr<Platform> platform_;
  HwSwitch* hw_;
};

} // namespace facebook::fboss
