// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"

#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <gtest/gtest.h>

namespace {
folly::CIDRNetwork kDefaultRoute{folly::IPAddressV6(), 0};
}

namespace facebook::fboss {

using utility::addAggPort;
using utility::disableTrunkPort;
using utility::EcmpSetupTargetedPorts6;
using utility::enableTrunkPorts;
using utility::getEcmpSizeInHw;

class HwEcmpTrunkTest : public HwLinkStateDependentTest {
 protected:
  std::vector<int32_t> getTrunkMemberPorts() const {
    return {
        (masterLogicalPortIds()[kEcmpWidth - 1]),
        (masterLogicalPortIds()[kEcmpWidth])};
  }

  flat_set<PortDescriptor> getEcmpPorts() const {
    flat_set<PortDescriptor> ports;
    for (auto i = 0; i < kEcmpWidth - 1; ++i) {
      ports.insert(PortDescriptor(PortID(masterLogicalPortIds()[i])));
    }
    ports.insert(PortDescriptor(kAggId));
    return ports;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ =
        std::make_unique<EcmpSetupTargetedPorts6>(getProgrammedState());
  }

  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::oneL3IntfNPortConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    addAggPort(kAggId, getTrunkMemberPorts(), &config);
    return config;
  }

  void runEcmpWithTrunkMinLinks(uint8_t minlinks) {
    auto setup = [=]() {
      auto state = enableTrunkPorts(getProgrammedState());
      applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
      applyNewState(ecmpHelper_->resolveNextHops(
          ecmpHelper_->setupECMPForwarding(
              getProgrammedState(), getEcmpPorts()),
          getEcmpPorts()));
    };

    auto verify = [=]() {
      // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
      // the ECMP group for it
      ASSERT_EQ(
          kEcmpWidth,
          getEcmpSizeInHw(
              getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));
      bringDownPort(PortID(getTrunkMemberPorts()[0]));
      auto numEcmpMembersToExclude = minlinks >= 2 ? 1 : 0;
      // Ecmp should shrink based on the aggCount if the number of ports
      // in Agg is less than min-links.
      ASSERT_EQ(
          kEcmpWidth - numEcmpMembersToExclude,
          getEcmpSizeInHw(
              getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

      bringUpPort(PortID(getTrunkMemberPorts()[0]));
      applyNewState(
          ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
      // Ecmp should expand to 7 since the number of ports in Agg is more than
      // min-links.
      ASSERT_EQ(
          kEcmpWidth,
          getEcmpSizeInHw(
              getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  const int kEcmpWidth = 7;
  const AggregatePortID kAggId{1};
  std::unique_ptr<EcmpSetupTargetedPorts6> ecmpHelper_;
};

// ECMP with trunk as member. One trunk member transitions to
// down state but no ECMP shrink happens because minlinks
// condition is not violated.
TEST_F(HwEcmpTrunkTest, TrunkNotRemovedFromEcmpWithMinLinks) {
  runEcmpWithTrunkMinLinks(1);
}

// Test removal of trunk members with routes resolving over it.
// This test performs member removal and add in a loop in an attempt
// to see whether we can recreate EBUSY error return from SDK which
// is seen on HwEcmpTrunkTest switches.
TEST_F(HwEcmpTrunkTest, TrunkMemberRemoveWithRouteTest) {
  uint8_t minlinks = 2;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));

    std::vector<RoutePrefixV6> v6Prefixes = {
        RoutePrefixV6{folly::IPAddressV6("1000::1"), 48},
        RoutePrefixV6{folly::IPAddressV6("1001::1"), 48},
        RoutePrefixV6{folly::IPAddressV6("1002::1"), 48},
        RoutePrefixV6{folly::IPAddressV6("1003::1"), 48},
        RoutePrefixV6{folly::IPAddressV6("1004::1"), 64},
        RoutePrefixV6{folly::IPAddressV6("1005::1"), 64},
        RoutePrefixV6{folly::IPAddressV6("1006::1"), 64},
        RoutePrefixV6{folly::IPAddressV6("1007::1"), 128},
        RoutePrefixV6{folly::IPAddressV6("1008::1"), 128},
        RoutePrefixV6{folly::IPAddressV6("1009::1"), 128},
    };

    applyNewState(ecmpHelper_->resolveNextHops(
        ecmpHelper_->setupECMPForwarding(
            getProgrammedState(), getEcmpPorts(), v6Prefixes),
        getEcmpPorts()));
  };

  auto verify = [=]() {
    auto trunkMembers = getTrunkMemberPorts();
    auto trunkMemberCount = trunkMembers.size();
    auto activeMemberCount = trunkMemberCount;
    for (const auto& member : trunkMembers) {
      utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);
      auto state =
          disableTrunkPort(getProgrammedState(), kAggId, PortID(member));
      applyNewState(state);
      activeMemberCount--;
      utility::verifyAggregatePortMemberCount(
          getHwSwitchEnsemble(), kAggId, trunkMemberCount, activeMemberCount);
    }
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(state);
    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, trunkMemberCount, trunkMemberCount);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
