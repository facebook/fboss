/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/tests/BcmTrunkUtils.h"

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <boost/container/flat_set.hpp>

using boost::container::flat_set;
using folly::IPAddressV6;
namespace {

facebook::fboss::RoutePrefix<IPAddressV6> kDefaultRoute{folly::IPAddressV6(),
                                                        0};
}

namespace facebook::fboss {

using utility::addAggPort;
using utility::disableTrunkPort;
using utility::EcmpSetupTargetedPorts6;
using utility::enableTrunkPorts;
using utility::getEcmpSizeInHw;
using utility::getEgressIdForRoute;

class BcmEcmpTrunkTest : public BcmLinkStateDependentTests {
 protected:
  std::vector<int32_t> getTrunkMemberPorts() const {
    return {(masterLogicalPortIds()[kEcmpWidth - 1]),
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
    BcmLinkStateDependentTests::SetUp();
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
    auto ecmpShrinkTest = (minlinks >= 2) ? true : false;
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
              getUnit(),
              getEgressIdForRoute(
                  getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
              kEcmpWidth));
      bringDownPort(PortID(getTrunkMemberPorts()[0]));
      auto numEcmpMembersToExclude = minlinks >= 2 ? 1 : 0;
      // Ecmp should shrink based on the aggCount if the number of ports
      // in Agg is less than min-links.
      ASSERT_EQ(
          kEcmpWidth - numEcmpMembersToExclude,
          getEcmpSizeInHw(
              getUnit(),
              getEgressIdForRoute(
                  getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
              kEcmpWidth));
      bringUpPort(PortID(getTrunkMemberPorts()[0]));
      applyNewState(
          ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
      // Ecmp should expand to 7 since the number of ports in Agg is more than
      // min-links.
      ASSERT_EQ(
          kEcmpWidth,
          getEcmpSizeInHw(
              getUnit(),
              getEgressIdForRoute(
                  getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
              kEcmpWidth));
    };
    // TODO - Enable warm boot verification in ECMP shrink case
    if (ecmpShrinkTest) {
      setup();
      verify();
    } else {
      verifyAcrossWarmBoots(setup, verify);
    }
  }

  const int kEcmpWidth = 7;
  const AggregatePortID kAggId{1};
  std::unique_ptr<EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(BcmEcmpTrunkTest, TrunkNotRemovedFromEcmpWithMinLinks) {
  runEcmpWithTrunkMinLinks(1);
}

// Test removal of trunk members with routes resolving over it.
// This test performs member removal and add in a loop in an attempt
// to see whether we can recreate EBUSY error return from SDK which
// is seen on production switches.
TEST_F(BcmEcmpTrunkTest, TrunkMemberRemoveWithRouteTest) {
  uint8_t minlinks = 2;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));

    std::vector<RoutePrefixV6> v6Prefixes = {
        RoutePrefixV6{IPAddressV6("1000::1"), 48},
        RoutePrefixV6{IPAddressV6("1001::1"), 48},
        RoutePrefixV6{IPAddressV6("1002::1"), 48},
        RoutePrefixV6{IPAddressV6("1003::1"), 48},
        RoutePrefixV6{IPAddressV6("1004::1"), 64},
        RoutePrefixV6{IPAddressV6("1005::1"), 64},
        RoutePrefixV6{IPAddressV6("1006::1"), 64},
        RoutePrefixV6{IPAddressV6("1007::1"), 128},
        RoutePrefixV6{IPAddressV6("1008::1"), 128},
        RoutePrefixV6{IPAddressV6("1009::1"), 128},
    };

    applyNewState(ecmpHelper_->resolveNextHops(
        ecmpHelper_->setupECMPForwarding(
            getProgrammedState(), getEcmpPorts(), v6Prefixes),
        getEcmpPorts()));
  };

  auto verify = [=]() {
    auto trunkMembers = getTrunkMemberPorts();
    auto trunkMemberCount = trunkMembers.size();
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    auto activeMemberCount = trunkMemberCount;
    for (const auto& member : trunkMembers) {
      EXPECT_EQ(
          activeMemberCount,
          utility::getBcmTrunkMemberCount(
              getUnit(), trunkTable->getBcmTrunkId(kAggId), trunkMemberCount));
      auto state =
          disableTrunkPort(getProgrammedState(), kAggId, PortID(member));
      applyNewState(state);
      activeMemberCount--;
      EXPECT_EQ(
          activeMemberCount,
          utility::getBcmTrunkMemberCount(
              getUnit(), trunkTable->getBcmTrunkId(kAggId), trunkMemberCount));
    }
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(state);
    EXPECT_EQ(
        trunkMemberCount,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), trunkMemberCount));
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
