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
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    addAggPort(kAggId, getTrunkMemberPorts(), &config);
    return config;
  }

  void runEcmpWithTrunkMinLinks(uint8_t minlinks) {
    auto setup = [=]() {
      auto state = enableTrunkPorts(getProgrammedState());
      applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
      applyNewState(
          ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
      ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts());
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

    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts(), v6Prefixes);
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

// ECMP with trunk as member. One trunk member transitions to
// Down state before warmboot resulting in ECMP shrink. Post warmboot,
// ECMP stays shrunk. Port transitions to Up after warm boot
// but ECMP stays shrunk until LACP brings it up.
TEST_F(
    HwEcmpTrunkTest,
    TrunkL2ResolveNhopThenLinkAndLACPDownThenUpThenStateUp) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);

    // link down on member port resulting in ECMP shrink
    bringDownPort(PortID(getTrunkMemberPorts()[0]));

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(),
        kAggId,
        minlinks,
        minlinks - numEcmpMembersToExclude);

    // LACP watches on port down and change state
    state = disableTrunkPort(
        getProgrammedState(), kAggId, PortID(getTrunkMemberPorts()[0]));
    applyNewState(state);
  };

  auto verify = [=]() {
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);
    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(),
        kAggId,
        minlinks,
        minlinks - numEcmpMembersToExclude);
  };

  auto setupPostWarmboot = [=]() {
    // neighbor gets removed due to  link toggle
    applyNewState(ecmpHelper_->unresolveNextHops(
        getProgrammedState(), {PortDescriptor(kAggId)}));
    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
    // ECMP should stay shrunk
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(),
        kAggId,
        minlinks,
        minlinks - numEcmpMembersToExclude);
  };

  auto verifyPostWarmboot = [=]() {
    // LACP enables link post warm boot
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(state);
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    // ECMP should expand
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

// ECMP with trunk as member. One trunk member transitions to
// Down state before warmboot resulting in ECMP shrink.
// Warmboot happens before LACP reacts to link down resulting
// in mismatched trunk membership in hardware/software.
// During warm boot init, the trunk is add to ECMP for a breif
// time period till port states are re-applied. Post warmboot,
// ECMP stays shrunk. Port transitions to Up after warm boot
// but ECMP stays shrunk until LACP brings it up.
// For details regarding traffic loss during short period
// of time during warmboot init, please check the wiki -
//
// https://github.com/facebook/fboss/blob/master/fboss/agent/wiki/warmboot_link_flaps.rst
TEST_F(HwEcmpTrunkTest, TrunkL2ResolveNhopThenLinkDownThenUpThenStateUp) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);
    // link down on member port resulting in ECMP shrink
    // LACP does not get a chance to react to port down before warm boot
    bringDownPort(PortID(getTrunkMemberPorts()[0]));
  };

  auto verify = [=]() {
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
  };

  auto setupPostWarmboot = [=]() {
    // neighbor gets removed due to  link toggle
    applyNewState(ecmpHelper_->unresolveNextHops(
        getProgrammedState(), {PortDescriptor(kAggId)}));

    // LACP reacts to link down notification at end of warm boot init
    auto state = disableTrunkPort(
        getProgrammedState(), kAggId, PortID(getTrunkMemberPorts()[0]));
    applyNewState(state);

    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
    // ECMP should stay shrunk

    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
  };

  auto verifyPostWarmboot = [=]() {
    // LACP enables link when state machine converges
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(state);
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    // ECMP should expand
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

// ECMP with trunk as member. One trunk member undergo
// quick port Up/Down transition resulting in ECMP shrink.
// LACP listens to port down and removes member from Aggport
// but warm boot shut down happens before ARP/ND reacts.
// On warm boot initialization, restoring neighbor entry
// should not expand ECMP since minlink condition is not met.
// LACP enables link post warm boot resulting in ECMP expansion.
TEST_F(
    HwEcmpTrunkTest,
    TrunkL2ResolveNhopThenLinkAndLACPDownThenUpBeforeReinit) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);

    // link down on member port resulting in ECMP shrink
    bringDownPort(PortID(getTrunkMemberPorts()[0]));
    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
    // LACP watches on port down and change state
    state = disableTrunkPort(
        getProgrammedState(), kAggId, PortID(getTrunkMemberPorts()[0]));
    // link state processing should remove neighbor
    state = ecmpHelper_->unresolveNextHops(
        state, {PortDescriptor(AggregatePortID(kAggId))});
    applyNewState(state);
    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
  };

  auto verify = [=]() {
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
  };

  auto setupPostWarmboot = [=]() {};

  auto verifyPostWarmboot = [=]() {
    // LACP enables link post warm boot
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(state);
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    // ECMP should expand
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

// ECMP with trunk as member. One trunk member experience
// quick link Down/Up transition resulting in ECMP shrink.
// Warmboot happens before LACP reacts to link down resulting
// in mismatched trunk membership in hardware/software.
// Warm boot restore results in ECMP expanding since LACP and
// ARP did not get any down event.
TEST_F(HwEcmpTrunkTest, TrunkL2ResolveNhopThenLinkUpDownUpBeforeReinit) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdater(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);

    // link down on member port resulting in ECMP shrink
    bringDownPort(PortID(getTrunkMemberPorts()[0]));

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, 1);
  };

  auto verify = [=]() {};

  auto setupPostWarmboot = [=]() {};

  auto verifyPostWarmboot = [=]() {
    // ECMP should expand on warmboot restore since ARP/ND entry is
    // present and port is up.
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(getHwSwitch(), kDefaultRoute, RouterID(0), kEcmpWidth));

    utility::verifyPktFromAggregatePort(getHwSwitchEnsemble(), kAggId);

    utility::verifyAggregatePortMemberCount(
        getHwSwitchEnsemble(), kAggId, minlinks, minlinks);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}
} // namespace facebook::fboss
