/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"

namespace {
facebook::fboss::RoutePrefixV6 kDefaultRoute{folly::IPAddressV6(), 0};
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};
} // namespace
namespace facebook::fboss {

class HwEcmpTest : public HwLinkStateDependentTest {
 protected:
  std::vector<NextHopWeight> swSwitchWeights_ = {ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT};
  std::vector<NextHopWeight> hwSwitchWeights_ = {UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT};
  const RouterID kRid{0};
  static constexpr auto kNumNextHops{8};
  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), kRid);
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
  }

  void resolveNhops(int numNhops) {
    applyNewState(ecmpHelper_->resolveNextHops(getProgrammedState(), numNhops));
  }
  void resolveNhops(const std::vector<PortDescriptor>& portDescs) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        flat_set<PortDescriptor>(portDescs.begin(), portDescs.end())));
  }
  void unresolveNhops(int numNhops) {
    applyNewState(
        ecmpHelper_->unresolveNextHops(getProgrammedState(), numNhops));
  }
  void unresolveNhops(const std::vector<PortDescriptor>& portDescs) {
    applyNewState(ecmpHelper_->unresolveNextHops(
        getProgrammedState(),
        flat_set<PortDescriptor>(portDescs.begin(), portDescs.end())));
  }
  void programRouteWithUnresolvedNhops(int numNhops = kNumNextHops) {
    applyNewState(ecmpHelper_->setupECMPForwarding(
        getProgrammedState(),
        numNhops,
        {kDefaultRoute},
        std::vector<NextHopWeight>(
            swSwitchWeights_.begin(), swSwitchWeights_.begin() + numNhops)));
  }
  int getEcmpSizeInHw(int sizeInSw = kNumNextHops) const {
    return utility::getEcmpSizeInHw(
        getHwSwitch(), kDefaultRoutePrefix, kRid, sizeInSw);
  }
  void runSimpleUcmpTest(
      const std::vector<NextHopWeight>& swWs,
      const std::vector<NextHopWeight>& hwWs);
  void programResolvedUcmp(
      const std::vector<NextHopWeight>& swWs,
      const std::vector<NextHopWeight>& hwWs);
  void verifyResolvedUcmp(const std::vector<NextHopWeight>& hwWs);
};

void HwEcmpTest::programResolvedUcmp(
    const std::vector<NextHopWeight>& swWs,
    const std::vector<NextHopWeight>& hwWs) {
  EXPECT_EQ(swWs.size(), hwWs.size());
  EXPECT_LE(swWs.size(), kNumNextHops);
  for (uint8_t i = 0; i < swWs.size(); ++i) {
    swSwitchWeights_[i] = swWs[i];
    hwSwitchWeights_[i] = hwWs[i];
  }
  programRouteWithUnresolvedNhops(swWs.size());
  resolveNhops(swWs.size());
}

void HwEcmpTest::verifyResolvedUcmp(const std::vector<NextHopWeight>& hwWs) {
  auto pathsInHw = utility::getEcmpMembersInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  auto pathsInHwCount = pathsInHw.size();
  EXPECT_LE(pathsInHwCount, FLAGS_ecmp_width);

  std::set<uint64_t> uniquePaths(pathsInHw.begin(), pathsInHw.end());
  // This check assumes that egress ids grow as you add more egresses
  // That assumption could prove incorrect, in which case we would
  // need to map ips to egresses, somehow.
  auto expectedCountIter = hwWs.begin();
  for (const auto& path : uniquePaths) {
    auto count = pathsInHw.count(path);
    EXPECT_EQ(count, *expectedCountIter);
    expectedCountIter++;
  }
  auto totalHwWeight =
      std::accumulate(hwWs.begin(), hwWs.end(), NextHopWeight(0));
  EXPECT_EQ(totalHwWeight, pathsInHwCount);
}

void HwEcmpTest::runSimpleUcmpTest(
    const std::vector<NextHopWeight>& swWs,
    const std::vector<NextHopWeight>& hwWs) {
  EXPECT_EQ(swWs.size(), hwWs.size());
  EXPECT_LE(swWs.size(), kNumNextHops);
  auto setup = [this, &swWs, &hwWs]() { programResolvedUcmp(swWs, hwWs); };
  auto verify = [this, &hwWs]() { verifyResolvedUcmp(hwWs); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwEcmpTest, L2ResolveOneNhopThenLinkDownThenUp) {
  auto setup = [=]() {
    programRouteWithUnresolvedNhops();
    resolveNhops(1);

    EXPECT_EQ(1, getEcmpSizeInHw());

    auto nhop = ecmpHelper_->nhop(0);
    bringDownPort(nhop.portDesc.phyPortID());
  };

  auto verify = [=]() {
    // ECMP shrunk on port down
    EXPECT_EQ(0, getEcmpSizeInHw());
  };

  auto setupPostWarmboot = [=]() {
    auto nhop = ecmpHelper_->nhop(0);
    bringUpPort(nhop.portDesc.phyPortID());
  };

  auto verifyPostWarmboot = [=]() {
    auto nhop = ecmpHelper_->nhop(0);
    // ECMP stays shrunk on port up
    EXPECT_EQ(0, getEcmpSizeInHw());

    // Bring port back down so we can warmboot more than once. This is
    // necessary because verify() and verifyPostWarmboot() assume that the port
    // is down and the nexthop unresolved, which won't be true if we warmboot
    // after bringing the port up in setupPostWarmboot().
    bringDownPort(nhop.portDesc.phyPortID());
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(HwEcmpTest, L2ResolveOneNhopThenLinkDownThenUpThenL2ResolveNhop) {
  auto setup = [=]() {
    programRouteWithUnresolvedNhops();
    auto nhop = ecmpHelper_->nhop(0);
    resolveNhops(1);
    EXPECT_EQ(1, getEcmpSizeInHw());
    bringDownPort(nhop.portDesc.phyPortID());
    // ECMP shrunk on port down
    EXPECT_EQ(0, getEcmpSizeInHw());
    bringUpPort(nhop.portDesc.phyPortID());
    // ECMP stays shrunk on port up
    EXPECT_EQ(0, getEcmpSizeInHw());
    // Re resolve nhop1
    unresolveNhops(1);
    resolveNhops(1);
  };

  auto verify = [=]() {
    // ECMP  expands post resolution
    EXPECT_EQ(1, getEcmpSizeInHw());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwEcmpTest, L2UnresolvedNhopsECMPInHWEmpty) {
  auto setup = [=]() { programRouteWithUnresolvedNhops(); };
  auto verify = [=]() { EXPECT_EQ(0, getEcmpSizeInHw()); };
  verifyAcrossWarmBoots(setup, verify);
}

// Test what happens when totalWeight > 64 in UCMP and some of the
// weights are too low, resulting in them going to zero when
// multiplied by 64/W (where W is the total weight of the nexthops).
// TODO(borisb): Think of a better algorithm for this case than wi*(64/W)
TEST_F(HwEcmpTest, UcmpOverflowZero) {
  EXPECT_EQ(64, FLAGS_ecmp_width);
  runSimpleUcmpTest({50, 50, 1, 1}, {31, 31, 1, 1});
}
TEST_F(HwEcmpTest, UcmpOverflowZeroNotEnoughToRoundUp) {
  EXPECT_EQ(64, FLAGS_ecmp_width);
  runSimpleUcmpTest({50, 50, 1, 1, 1, 1, 1, 1}, {29, 29, 1, 1, 1, 1, 1, 1});
}

TEST_F(HwEcmpTest, ResolvePendingResolveNexthop) {
  auto setup = [=]() {
    resolveNhops(2);
    std::map<PortDescriptor, std::shared_ptr<NdpEntry>> entries;

    // mark neighbors connected over ports pending
    auto state0 = getProgrammedState();
    for (auto i = 0; i < 2; i++) {
      const auto& ecmpNextHop = ecmpHelper_->nhop(i);
      auto port = ecmpNextHop.portDesc;
      auto vlanId = ecmpHelper_->getVlan(port);
      auto ntable = state0->getVlans()->getVlan(*vlanId)->getNdpTable()->modify(
          *vlanId, &state0);
      auto entry = ntable->getEntry(ecmpNextHop.ip);
      auto intfId = entry->getIntfID();
      ntable->removeEntry(ecmpNextHop.ip);
      ntable->addPendingEntry(ecmpNextHop.ip, intfId);
      entries[port] = std::move(entry);
    }
    applyNewState(state0);

    // mark neighbors connected over ports reachable
    auto state1 = getProgrammedState();
    for (auto i = 0; i < 2; i++) {
      const auto& ecmpNextHop = ecmpHelper_->nhop(i);
      auto port = ecmpNextHop.portDesc;
      auto vlanId = ecmpHelper_->getVlan(port);
      auto ntable = state1->getVlans()->getVlan(*vlanId)->getNdpTable()->modify(
          *vlanId, &state1);

      auto entry = entries[port];
      auto* fields = entry->getFields();
      ntable->updateEntry(*fields);
    }
    applyNewState(state1);
    applyNewState(ecmpHelper_->setupECMPForwarding(getProgrammedState(), 2));
  };
  auto verify = [=]() {
    /* ecmp is resolved */
    EXPECT_EQ(
        utility::getEcmpSizeInHw(getHwSwitch(), kDefaultRoutePrefix, kRid, 2),
        2);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Test link down in UCMP scenario
TEST_F(HwEcmpTest, UcmpL2ResolveAllNhopsInThenLinkDown) {
  programResolvedUcmp({3, 1, 1, 1, 1, 1, 1, 1}, {3, 1, 1, 1, 1, 1, 1, 1});
  bringDownPort(ecmpHelper_->nhop(0).portDesc.phyPortID());
  auto pathsInHwCount = utility::getEcmpSizeInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  EXPECT_EQ(7, pathsInHwCount);
}

// Test link flap in UCMP scenario
TEST_F(HwEcmpTest, UcmpL2ResolveBothNhopsInThenLinkFlap) {
  programResolvedUcmp({3, 1, 1, 1, 1, 1, 1, 1}, {3, 1, 1, 1, 1, 1, 1, 1});
  auto nhop = ecmpHelper_->nhop(0);
  bringDownPort(nhop.portDesc.phyPortID());
  auto pathsInHwCount = utility::getEcmpSizeInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  EXPECT_EQ(7, pathsInHwCount);
  bringUpPort(nhop.portDesc.phyPortID());
  pathsInHwCount = utility::getEcmpSizeInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  EXPECT_EQ(7, pathsInHwCount);
  unresolveNhops(1);
  resolveNhops(1);
  pathsInHwCount = utility::getEcmpSizeInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  EXPECT_EQ(10, pathsInHwCount);
}
} // namespace facebook::fboss
