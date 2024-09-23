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

#include "fboss/agent/Constants.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/StateDelta.h"
#include "folly/MacAddress.h"

#include <gmock/gmock.h>
#include <memory>
#include <optional>

namespace facebook::fboss {

/*
 * MockPlatform is a mockable interface to a Platform. Non-critical
 * functions have stub implementations and functions that we need to
 * control for tests are mocked with gmock.
 */
class MockHwSwitch : public HwSwitch {
 public:
  explicit MockHwSwitch(MockPlatform* platform);

  MOCK_METHOD3(initImpl, HwInitResult(Callback*, BootType, bool));
  MOCK_METHOD1(
      stateChangedImpl,
      std::shared_ptr<SwitchState>(const StateDelta& delta));
  MOCK_METHOD2(
      stateChangedTransaction,
      std::shared_ptr<SwitchState>(
          const StateDelta& delta,
          const HwWriteBehaviorRAII&));
  MOCK_CONST_METHOD0(reconstructSwitchState, std::shared_ptr<SwitchState>());

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  void printDiagCmd(const std::string& cmd) const override;

  // gmock currently doesn't support move-only types, so we have to
  // use some janky work-arounds.
  MOCK_METHOD1(sendPacketSwitchedAsync_, bool(TxPacket*));
  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;

  MOCK_METHOD3(
      sendPacketOutOfPortAsync_,
      bool(TxPacket*, facebook::fboss::PortID, std::optional<uint8_t> queue));
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      facebook::fboss::PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;
  MOCK_METHOD1(sendPacketSwitchedSync_, bool(TxPacket*));
  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  MOCK_METHOD3(
      sendPacketOutOfPortSync_,
      bool(TxPacket*, facebook::fboss::PortID, std::optional<uint8_t> queue));
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      facebook::fboss::PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  MOCK_CONST_METHOD0(transactionsSupported, bool());
  MOCK_METHOD0(updateStatsImpl, void());
  MOCK_CONST_METHOD0(
      getPortStats,
      folly::F14FastMap<std::string, HwPortStats>());
  MOCK_CONST_METHOD0(getCpuPortStats, CpuPortStats());
  MOCK_CONST_METHOD0(getSysPortStats, std::map<std::string, HwSysPortStats>());
  MOCK_CONST_METHOD0(getFabricReachabilityStats, FabricReachabilityStats());
  MOCK_CONST_METHOD0(getSwitchDropStats, HwSwitchDropStats());
  MOCK_CONST_METHOD1(fetchL2Table, void(std::vector<L2EntryThrift>* l2Table));
  MOCK_METHOD0(gracefulExitImpl, void());
  MOCK_CONST_METHOD0(toFollyDynamic, folly::dynamic());
  MOCK_CONST_METHOD0(exitFatal, void());
  MOCK_METHOD0(unregisterCallbacks, void());
  MOCK_CONST_METHOD1(isValidStateUpdate, bool(const StateDelta& delta));
  MOCK_CONST_METHOD1(isPortUp, bool(PortID port));
  MOCK_CONST_METHOD1(getPortMaxSpeed, cfg::PortSpeed(PortID port));
  MOCK_METHOD1(
      clearPortStats,
      void(const std::unique_ptr<std::vector<int32_t>>&));
  MOCK_CONST_METHOD0(getBootType, BootType());
  MOCK_CONST_METHOD0(getTeFlowStats, TeFlowStats());
  MOCK_CONST_METHOD0(getHwFlowletStats, HwFlowletStats());
  MOCK_CONST_METHOD0(getAllEcmpDetails, std::vector<EcmpDetails>());
  MOCK_CONST_METHOD0(getAclStats, AclStats());
  MOCK_CONST_METHOD0(getSwitchWatermarkStats, HwSwitchWatermarkStats());

  MockPlatform* getPlatform() const override {
    return platform_;
  }
  MOCK_CONST_METHOD1(dumpDebugState, void(const std::string& path));
  /*
   * Return true to allow testing for static l2 entry creation
   * code paths in unit tests
   */
  bool needL2EntryForNeighbor() const override {
    return true;
  }

  uint64_t getDeviceWatermarkBytes() const override {
    return 0;
  }

  void injectSwitchReachabilityChangeNotification() override {}

  uint32_t generateDeterministicSeed(
      LoadBalancerID loadBalancerID,
      folly::MacAddress mac) const override {
    auto mac64 = mac.u64HBO();
    uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);

    uint32_t seed = 0;
    switch (loadBalancerID) {
      case LoadBalancerID::ECMP:
        seed = folly::hash::jenkins_rev_mix32(mac32);
        break;
      case LoadBalancerID::AGGREGATE_PORT:
        seed = folly::hash::twang_32from64(mac64);
        break;
    }
    return seed;
  }

  MOCK_CONST_METHOD2(
      listObjects,
      std::string(const std::vector<HwObjectType>&, bool));
  MOCK_METHOD0(updateAllPhyInfoImpl, std::map<PortID, phy::PhyInfo>());
  MOCK_CONST_METHOD0(
      getFabricConnectivity,
      const std::map<PortID, FabricEndpoint>&());
  MOCK_CONST_METHOD1(
      getSwitchReachability,
      std::vector<PortID>(SwitchID switchId));

  void setInitialState(const std::shared_ptr<SwitchState>& state) {
    setProgrammedState(state);
  }

  MOCK_CONST_METHOD0(getResourceStats, HwResourceStats());

 private:
  MOCK_METHOD1(switchRunStateChangedImpl, void(SwitchRunState newState));
  MOCK_METHOD0(initialStateApplied, void());
  MOCK_METHOD0(syncLinkStates, void());
  MOCK_METHOD0(syncLinkActiveStates, void());
  MOCK_METHOD0(syncLinkConnectivity, void());
  MOCK_METHOD0(syncSwitchReachability, void());

  MockPlatform* platform_;

  // Forbidden copy constructor and assignment operator
  MockHwSwitch(MockHwSwitch const&) = delete;
  MockHwSwitch& operator=(MockHwSwitch const&) = delete;
};

} // namespace facebook::fboss
