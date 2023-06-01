// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace facebook::fboss {
class HwTrunkTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackModes());
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& switchCfg) {
    auto state = applyNewConfig(switchCfg);
    applyNewState(utility::enableTrunkPorts(state));
  }
};

TEST_F(HwTrunkTest, TrunkCreateHighLowKeyIds) {
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    utility::verifyAggregatePortCount(getHwSwitchEnsemble(), 2);
    utility::verifyAggregatePort(
        getHwSwitchEnsemble(), AggregatePortID(1)

    );
    utility::verifyAggregatePort(
        getHwSwitchEnsemble(),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max())

    );
    auto aggIDs = {
        AggregatePortID(1),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max())};
    for (auto aggId : aggIDs) {
      utility::verifyAggregatePortMemberCount(
          getHwSwitchEnsemble(), aggId, 1, 1);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkCheckIngressPktAggPort) {
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    utility::verifyPktFromAggregatePort(
        getHwSwitchEnsemble(),
        AggregatePortID(std::numeric_limits<AggregatePortID>::max()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkMemberPortDownMinLinksViolated) {
  auto aggId = AggregatePortID(std::numeric_limits<AggregatePortID>::max());

  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(
        aggId, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);

    bringDownPort(PortID(masterLogicalPortIds()[0]));
    // Member port count should drop to 1 now.
  };
  auto verify = [=]() {
    utility::verifyAggregatePortCount(getHwSwitchEnsemble(), 1);
    utility::verifyAggregatePort(getHwSwitchEnsemble(), aggId);
    utility::verifyAggregatePortMemberCount(getHwSwitchEnsemble(), aggId, 2, 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkPortStatsWithMplsPush) {
  if (getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_FAKE ||
      getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_MOCK ||
      getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_ELBERT_8DD) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
    auto ecmpHelper = utility::EcmpSetupTargetedPorts6(getProgrammedState());
    applyNewState(ecmpHelper.resolveNextHops(
        getProgrammedState(), {PortDescriptor(AggregatePortID(1))}));
    ecmpHelper.programIp2MplsRoutes(
        getRouteUpdater(),
        {PortDescriptor(AggregatePortID(1))},
        {{PortDescriptor(AggregatePortID(1)), {1001, 1002}}});
  };
  auto verify = [=]() {
    auto vlanId = VlanID(utility::kBaseVlanId);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    for (auto throughPort : {false, true}) {
      auto stats = getLatestPortStats(masterLogicalPortIds()[1]);
      auto pkts0 = *stats.outUnicastPkts_() + *stats.outMulticastPkts_() +
          *stats.outBroadcastPkts_();
      auto trunkStats = getLatestAggregatePortStats(AggregatePortID(1));
      auto trunkPkts0 = *trunkStats.outUnicastPkts_() +
          *trunkStats.outMulticastPkts_() + *trunkStats.outBroadcastPkts_();
      auto pkt = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddress("2401::1"),
          folly::IPAddress("2401::2"),
          10001,
          20001);
      throughPort
          ? getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
                std::move(pkt), masterLogicalPortIds()[0])
          : getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(pkt));
      stats = getLatestPortStats(masterLogicalPortIds()[1]);
      trunkStats = getLatestAggregatePortStats(AggregatePortID(1));
      auto pkts1 = *stats.outUnicastPkts_() + *stats.outMulticastPkts_() +
          *stats.outBroadcastPkts_();
      auto trunkPkts1 = *trunkStats.outUnicastPkts_() +
          *trunkStats.outMulticastPkts_() + *trunkStats.outBroadcastPkts_();
      EXPECT_GT(pkts1, pkts0);
      EXPECT_GT(trunkPkts1, trunkPkts0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTrunkTest, TrunkPortStats) {
  if (getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_FAKE ||
      getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_MOCK ||
      getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_ELBERT_8DD) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    auto cfg = initialConfig();
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
    auto ecmpHelper = utility::EcmpSetupTargetedPorts6(getProgrammedState());
    applyNewState(ecmpHelper.resolveNextHops(
        getProgrammedState(), {PortDescriptor(AggregatePortID(1))}));
    ecmpHelper.programRoutes(
        getRouteUpdater(), {PortDescriptor(AggregatePortID(1))});
  };
  auto verify = [=]() {
    auto vlanId = VlanID(utility::kBaseVlanId);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    for (auto throughPort : {false, true}) {
      auto stats = getLatestPortStats(masterLogicalPortIds()[1]);
      auto pkts0 = *stats.outUnicastPkts_() + *stats.outMulticastPkts_() +
          *stats.outBroadcastPkts_();
      auto trunkStats = getLatestAggregatePortStats(AggregatePortID(1));
      auto trunkPkts0 = *trunkStats.outUnicastPkts_() +
          *trunkStats.outMulticastPkts_() + *trunkStats.outBroadcastPkts_();
      auto pkt = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddress("2401::1"),
          folly::IPAddress("2401::2"),
          10001,
          20001);
      throughPort
          ? getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
                std::move(pkt), masterLogicalPortIds()[0])
          : getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(pkt));
      stats = getLatestPortStats(masterLogicalPortIds()[1]);
      trunkStats = getLatestAggregatePortStats(AggregatePortID(1));
      auto pkts1 = *stats.outUnicastPkts_() + *stats.outMulticastPkts_() +
          *stats.outBroadcastPkts_();
      auto trunkPkts1 = *trunkStats.outUnicastPkts_() +
          *trunkStats.outMulticastPkts_() + *trunkStats.outBroadcastPkts_();
      EXPECT_GT(pkts1, pkts0);
      EXPECT_GT(trunkPkts1, trunkPkts0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
