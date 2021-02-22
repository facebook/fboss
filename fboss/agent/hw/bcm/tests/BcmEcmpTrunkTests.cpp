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

facebook::fboss::RoutePrefix<IPAddressV6> kDefaultRoute{
    folly::IPAddressV6(),
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
    auto setup = [=]() {
      auto state = enableTrunkPorts(getProgrammedState());
      applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
      applyNewState(
          ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
      ecmpHelper_->programRoutes(getRouteUpdateWrapper(), getEcmpPorts());
    };

    auto verify = [=]() {
      // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
      // the ECMP group for it
      ASSERT_EQ(
          kEcmpWidth,
          getEcmpSizeInHw(
              getHwSwitch(),
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
              getHwSwitch(),
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
              getHwSwitch(),
              getEgressIdForRoute(
                  getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
              kEcmpWidth));
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  const int kEcmpWidth = 7;
  const AggregatePortID kAggId{1};
  std::unique_ptr<EcmpSetupTargetedPorts6> ecmpHelper_;
};

// ECMP with trunk as member. One trunk member undergo
// quick port Up/Down transition resulting in ECMP shrink.
// LACP listens to port down and removes member from Aggport
// but warm boot shut down happens before ARP/ND reacts.
// On warm boot initialization, restoring neighbor entry
// should not expand ECMP since minlink condition is not met.
// LACP enables link post warm boot resulting in ECMP expansion.
TEST_F(
    BcmEcmpTrunkTest,
    TrunkL2ResolveNhopThenLinkAndLACPDownThenUpBeforeReinit) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdateWrapper(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    EXPECT_EQ(
        2,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
    // link down on member port resulting in ECMP shrink
    bringDownPort(PortID(getTrunkMemberPorts()[0]));
    EXPECT_EQ(
        1,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
    // LACP watches on port down and change state
    state = disableTrunkPort(
        getProgrammedState(), kAggId, PortID(getTrunkMemberPorts()[0]));
    applyNewState(state);
    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
  };

  auto verify = [=]() {
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    EXPECT_EQ(
        1,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
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
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    EXPECT_EQ(
        2,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

// ECMP with trunk as member. One trunk member experience
// quick link Down/Up transition resulting in ECMP shrink.
// Warmboot happens before LACP reacts to link down resulting
// in mismatched trunk membership in hardware/software.
// Warm boot restore results in ECMP expanding since LACP and
// ARP did not get any down event.
TEST_F(BcmEcmpTrunkTest, TrunkL2ResolveNhopThenLinkUpDownUpBeforeReinit) {
  uint8_t minlinks = 2;
  uint8_t numEcmpMembersToExclude = 1;
  auto setup = [=]() {
    auto state = enableTrunkPorts(getProgrammedState());
    applyNewState(utility::setTrunkMinLinkCount(state, minlinks));
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), getEcmpPorts()));
    ecmpHelper_->programRoutes(getRouteUpdateWrapper(), getEcmpPorts());
    // We programmed the default route picked by EcmpSetupAnyNPorts so lookup
    // the ECMP group for it
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    EXPECT_EQ(
        2,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
    // link down on member port resulting in ECMP shrink
    bringDownPort(PortID(getTrunkMemberPorts()[0]));
    EXPECT_EQ(
        1,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
    // bring up the member link
    bringUpPort(PortID(getTrunkMemberPorts()[0]));
    // Ecmp should shrink based on the aggCount since the number of ports
    // in Agg is less than min-links.
    ASSERT_EQ(
        kEcmpWidth - numEcmpMembersToExclude,
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    EXPECT_EQ(
        1,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
  };

  auto verify = [=]() {};

  auto setupPostWarmboot = [=]() {};

  auto verifyPostWarmboot = [=]() {
    // ECMP should expand on warmboot restore since ARP/ND entry is
    // present and port is up.
    ASSERT_EQ(
        kEcmpWidth,
        getEcmpSizeInHw(
            getHwSwitch(),
            getEgressIdForRoute(
                getHwSwitch(), kDefaultRoute.network, kDefaultRoute.mask),
            kEcmpWidth));
    auto trunkTable = getHwSwitch()->getTrunkTable();
    EXPECT_NO_THROW(trunkTable->getBcmTrunkId(kAggId));
    EXPECT_EQ(
        2,
        utility::getBcmTrunkMemberCount(
            getUnit(), trunkTable->getBcmTrunkId(kAggId), 2));
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
