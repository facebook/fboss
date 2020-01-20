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

#include "fboss/agent/hw/bcm/tests/BcmEcmpTests.h"
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
using utility::EcmpSetupTargetedPorts6;
using utility::enableTrunkPorts;
using utility::getEcmpSizeInHw;
using utility::getEgressIdForRoute;

class BcmEcmpTrunkTest : public BcmLinkStateDependentTests {
 private:
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

 protected:
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
    setup();
    verify();
  }

 private:
  const int kEcmpWidth = 7;
  const AggregatePortID kAggId{1};
  std::unique_ptr<EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(BcmEcmpTrunkTest, TrunkRemovedFromEcmpWithMinLinks) {
  runEcmpWithTrunkMinLinks(2);
}

TEST_F(BcmEcmpTrunkTest, TrunkNotRemovedFromEcmpWithMinLinks) {
  runEcmpWithTrunkMinLinks(1);
}

} // namespace facebook::fboss
