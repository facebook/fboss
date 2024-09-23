/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/sim/SimPlatform.h"

#include <optional>

namespace facebook::fboss {

class SwitchState;

class SimSwitch : public HwSwitch {
 public:
  SimSwitch(SimPlatform* platform, uint32_t numPorts);

  HwInitResult initImpl(
      Callback* callback,
      BootType bootType,
      bool failHwCallsOnWarmboot) override;
  std::shared_ptr<SwitchState> stateChangedImpl(
      const StateDelta& delta) override;
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;
  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;
  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  /**
   * Runs a diag cmd on the corresponding unit
   */
  void printDiagCmd(const std::string& cmd) const override;

  folly::dynamic toFollyDynamic() const override;

  void injectPacket(std::unique_ptr<RxPacket> pkt);

  folly::F14FastMap<std::string, HwPortStats> getPortStats() const override {
    return {};
  }
  std::map<std::string, HwSysPortStats> getSysPortStats() const override {
    return {};
  }
  CpuPortStats getCpuPortStats() const override {
    return {};
  }
  FabricReachabilityStats getFabricReachabilityStats() const override {
    return {};
  }

  TeFlowStats getTeFlowStats() const override {
    return TeFlowStats();
  }
  HwSwitchDropStats getSwitchDropStats() const override {
    return HwSwitchDropStats{};
  }
  HwFlowletStats getHwFlowletStats() const override {
    return HwFlowletStats{};
  }
  AclStats getAclStats() const override {
    return AclStats{};
  }

  HwSwitchWatermarkStats getSwitchWatermarkStats() const override {
    return HwSwitchWatermarkStats{};
  }

  HwResourceStats getResourceStats() const override {
    return HwResourceStats{};
  }

  std::vector<EcmpDetails> getAllEcmpDetails() const override {
    return {};
  }

  void fetchL2Table(std::vector<L2EntryThrift>* /*l2Table*/) const override {
    return;
  }

  void resetTxCount() {
    txCount_ = 0;
  }
  uint64_t getTxCount() const {
    return txCount_;
  }
  void exitFatal() const override {
    // TODO
  }

  void unregisterCallbacks() override {
    // TODO
  }

  bool isPortUp(PortID /*port*/) const override {
    // Should be called only from SwSwitch which knows whether
    // the port is enabled or not
    return true;
  }

  cfg::PortSpeed getPortMaxSpeed(PortID /* port */) const override {
    return cfg::PortSpeed::HUNDREDG;
  }

  bool isValidStateUpdate(const StateDelta& /*delta*/) const override {
    return true;
  }

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& /*ports*/) override {}

  virtual BootType getBootType() const override {
    return bootType_;
  }

  SimPlatform* getPlatform() const override {
    return platform_;
  }
  void dumpDebugState(const std::string& /*path*/) const override {}

  uint64_t getDeviceWatermarkBytes() const override {
    return 0;
  }

  std::string listObjects(
      const std::vector<HwObjectType>& /*types*/,
      bool /*cached*/) const override {
    return "";
  }

  std::map<PortID, phy::PhyInfo> updateAllPhyInfoImpl() override {
    return {};
  }
  const std::map<PortID, FabricEndpoint>& getFabricConnectivity()
      const override {
    static const std::map<PortID, FabricEndpoint> kEmpty;
    return kEmpty;
  }
  std::vector<PortID> getSwitchReachability(SwitchID switchId) const override {
    return {};
  }

  uint32_t generateDeterministicSeed(
      LoadBalancerID /*loadBalancerID*/,
      folly::MacAddress /*mac*/) const override {
    return 0;
  }

  void injectSwitchReachabilityChangeNotification() override {}

 private:
  void switchRunStateChangedImpl(SwitchRunState newState) override {}
  // TODO
  void updateStatsImpl() override {}

  void gracefulExitImpl() override {}

  void initialStateApplied() override {}

  void syncLinkStates() override {}
  void syncLinkActiveStates() override {}
  void syncLinkConnectivity() override {}
  void syncSwitchReachability() override {}

  std::shared_ptr<SwitchState> reconstructSwitchState() const override;

  // Forbidden copy constructor and assignment operator
  SimSwitch(SimSwitch const&) = delete;
  SimSwitch& operator=(SimSwitch const&) = delete;

  SimPlatform* platform_;
  HwSwitchCallback* callback_{nullptr};
  uint32_t numPorts_{0};
  uint64_t txCount_{0};
  BootType bootType_{BootType::UNINITIALIZED};
};

} // namespace facebook::fboss
