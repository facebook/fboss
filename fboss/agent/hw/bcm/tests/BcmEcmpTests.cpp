/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"

#include <folly/IPAddress.h>

#include <boost/container/flat_set.hpp>

#include <memory>
#include <numeric>
#include <set>

extern "C" {
#include <bcm/l3.h>
#include <bcm/port.h>
}

DECLARE_uint32(ecmp_width);

using boost::container::flat_set;
using facebook::fboss::utility::getEcmpGroupInHw;
using facebook::fboss::utility::getEcmpSizeInHw;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
facebook::fboss::RoutePrefixV6 kDefaultRoute{IPAddressV6(), 0};
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};
} // namespace
namespace facebook::fboss {

class BcmEcmpTest : public BcmLinkStateDependentTests {
 protected:
  const RouterID kRid{0};
  constexpr static uint8_t kNumNextHops{8};
  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
  void SetUp() override {
    BcmLinkStateDependentTests::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), kRid);
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getHwSwitch()->getPlatform()->getAsic()->desiredLoopbackModes());
  }

  void programRouteWithUnresolvedNhops();
  const BcmEcmpEgress* getEcmpEgress() const;
  const BcmMultiPathNextHop* getBcmMultiPathNextHop() const;
};

void BcmEcmpTest::programRouteWithUnresolvedNhops() {
  ecmpHelper_->programRoutes(getRouteUpdater(), kNumNextHops);
}

const BcmMultiPathNextHop* BcmEcmpTest::getBcmMultiPathNextHop() const {
  auto resolvedRoute = findRoute<folly::IPAddressV6>(
      kRid, kDefaultRoutePrefix, getProgrammedState());
  const auto multiPathTable = getHwSwitch()->getMultiPathNextHopTable();
  RouteNextHopSet nhops;
  std::unordered_map<IPAddress, NextHopWeight> ws;
  for (uint8_t i = 0; i < kNumNextHops; ++i) {
    ws[ecmpHelper_->ip(i)] = UCMP_DEFAULT_WEIGHT;
  }
  for (const auto& nhop : resolvedRoute->getForwardInfo().getNextHopSet()) {
    nhops.insert(ResolvedNextHop(nhop.addr(), nhop.intf(), ws[nhop.addr()]));
  }
  return multiPathTable->getNextHop(BcmMultiPathNextHopKey(kRid, nhops));
}

const BcmEcmpEgress* BcmEcmpTest::getEcmpEgress() const {
  return getBcmMultiPathNextHop()->getEgress();
}

TEST_F(BcmEcmpTest, SearchMissingEgressInECMP) {
  auto setup = [=]() { programRouteWithUnresolvedNhops(); };
  auto verify = [=]() {
    bcm_l3_egress_ecmp_t ecmpObj;
    bcm_l3_egress_ecmp_t_init(&ecmpObj);
    auto ecmpEgress = getEcmpEgress();
    auto egressIdsInSw = ecmpEgress->egressId2Weight();
    ASSERT_EQ(kNumNextHops, egressIdsInSw.size());
    ecmpObj.ecmp_intf = ecmpEgress->getID();
    for (const auto& egressId : egressIdsInSw) {
      int rv = 0;
      if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::HSDK)) {
        bcm_l3_ecmp_member_t member;
        bcm_l3_ecmp_member_t_init(&member);
        member.egress_if = egressId.first;
        rv = bcm_l3_ecmp_member_delete(getUnit(), ecmpEgress->getID(), &member);
      } else {
        rv = bcm_l3_egress_ecmp_delete(getUnit(), &ecmpObj, egressId.first);
      }
      ASSERT_EQ(BCM_E_NOT_FOUND, rv);
    }
    auto pathsInHwCount = getEcmpSizeInHw(
        getHwSwitch(), ecmpEgress->getID(), egressIdsInSw.size());
    ASSERT_EQ(0, pathsInHwCount);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmEcmpTest, UcmpShrink) {
  const std::vector<NextHopWeight> weights = {3, 1, 1, 1, 1, 1, 1, 1};
  // Program ECMP route
  applyNewState(
      ecmpHelper_->resolveNextHops(getProgrammedState(), kNumNextHops));
  ecmpHelper_->programRoutes(
      getRouteUpdater(), kNumNextHops, {kDefaultRoute}, weights);
  EXPECT_EQ(
      kNumNextHops + 2,
      getEcmpSizeInHw(
          getHwSwitch(), kDefaultRoutePrefix, kRid, kNumNextHops + 2));
  // Mimic neighbor entries going away and route getting removed.
  flat_set<PortDescriptor> weightedPorts;
  flat_set<PortDescriptor> unweightedPorts;
  weightedPorts.insert(ecmpHelper_->getNextHops().at(0).portDesc);
  for (int i = 1; i < kNumNextHops; i++) {
    unweightedPorts.insert(ecmpHelper_->getNextHops().at(i).portDesc);
  }
  applyNewState(
      ecmpHelper_->unresolveNextHops(getProgrammedState(), weightedPorts));
  EXPECT_EQ(
      kNumNextHops - 1,
      getEcmpSizeInHw(
          getHwSwitch(), kDefaultRoutePrefix, kRid, kNumNextHops + 2));
  applyNewState(
      ecmpHelper_->unresolveNextHops(getProgrammedState(), unweightedPorts));
  EXPECT_EQ(
      0,
      getEcmpSizeInHw(
          getHwSwitch(), kDefaultRoutePrefix, kRid, kNumNextHops + 2));
}

TEST_F(BcmEcmpTest, NativeUcmpFlag) {
  auto ucmpSupported = getHwSwitch()->getPlatform()->getAsic()->isSupported(
      HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER);
  // Program UCMP route
  const std::vector<NextHopWeight> ucmpWeights = {3, 1, 1, 1, 1, 1, 1, 1};
  applyNewState(
      ecmpHelper_->resolveNextHops(getProgrammedState(), kNumNextHops));
  ecmpHelper_->programRoutes(
      getRouteUpdater(), kNumNextHops, {kDefaultRoute}, ucmpWeights);
  auto ecmp = utility::getEgressIdForRoute(
      getHwSwitch(),
      kDefaultRoutePrefix.first,
      kDefaultRoutePrefix.second,
      kRid);
  EXPECT_EQ(ucmpSupported, utility::isNativeUcmpEnabled(getHwSwitch(), ecmp));
  // Program ECMP route
  const std::vector<NextHopWeight> ecmpWeights = {1, 1, 1, 1, 1, 1, 1, 1};
  applyNewState(
      ecmpHelper_->resolveNextHops(getProgrammedState(), kNumNextHops));
  ecmpHelper_->programRoutes(
      getRouteUpdater(), kNumNextHops, {kDefaultRoute}, ecmpWeights);
  ecmp = utility::getEgressIdForRoute(
      getHwSwitch(),
      kDefaultRoutePrefix.first,
      kDefaultRoutePrefix.second,
      kRid);
  EXPECT_FALSE(utility::isNativeUcmpEnabled(getHwSwitch(), ecmp));
}

} // namespace facebook::fboss
