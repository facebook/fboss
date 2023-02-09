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

facebook::fboss::RoutePrefixV6 kRoute2{
    folly::IPAddressV6{"2803:6080:d038:3063::"},
    64};
folly::CIDRNetwork kRoute2Prefix{folly::IPAddress("2803:6080:d038:3063::"), 64};
} // namespace
namespace facebook::fboss {

class HwEcmpTest : public HwLinkStateDependentTest {
 protected:
  std::vector<NextHopWeight> swSwitchWeights_ = {
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT};
  std::vector<NextHopWeight> hwSwitchWeights_ = {
      UCMP_DEFAULT_WEIGHT,
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
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
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
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        numNhops,
        {kDefaultRoute},
        std::vector<NextHopWeight>(
            swSwitchWeights_.begin(), swSwitchWeights_.begin() + numNhops));
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
  void verifyResolvedUcmp(
      const folly::CIDRNetwork& routePrefix,
      const std::vector<NextHopWeight>& hwWs);

  auto getNdpTable(PortDescriptor port, std::shared_ptr<SwitchState>& state) {
    auto switchType =
        getProgrammedState()->getSwitchSettings()->getSwitchType();
    if (switchType == cfg::SwitchType::NPU) {
      auto vlanId = ecmpHelper_->getVlan(port, getProgrammedState());
      return state->getVlans()->getVlan(*vlanId)->getNdpTable()->modify(
          *vlanId, &state);
    } else {
      auto portId = port.phyPortID();
      InterfaceID intfId(
          (*state->getPorts()->getPort(portId)->getInterfaceIDs()->begin())
              ->toThrift());
      return state->getInterfaces()
          ->getInterface(intfId)
          ->getNdpTable()
          ->modify(intfId, &state);
    }
  }
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

void HwEcmpTest::verifyResolvedUcmp(
    const folly::CIDRNetwork& routePrefix,
    const std::vector<NextHopWeight>& hwWs) {
  auto pathsInHw = utility::getEcmpMembersInHw(
      getHwSwitch(), routePrefix, kRid, FLAGS_ecmp_width);
  auto pathsInHwCount = pathsInHw.size();
  EXPECT_LE(pathsInHwCount, FLAGS_ecmp_width);

  std::set<uint64_t> uniquePaths(pathsInHw.begin(), pathsInHw.end());
  // This check assumes that egress ids grow as you add more egresses
  // That assumption could prove incorrect, in which case we would
  // need to map ips to egresses, somehow.
  auto expectedCountIter = hwWs.begin();
  for (const auto& path : uniquePaths) {
    auto pathWeight =
        utility::getEcmpMemberWeight(getHwSwitch(), pathsInHw, path);
    EXPECT_EQ(pathWeight, *expectedCountIter);
    expectedCountIter++;
  }
  auto totalHwWeight =
      std::accumulate(hwWs.begin(), hwWs.end(), NextHopWeight(0));
  auto totalWeightInHw =
      utility::getTotalEcmpMemberWeight(getHwSwitch(), pathsInHw);
  EXPECT_EQ(totalHwWeight, totalWeightInHw);
}

void HwEcmpTest::runSimpleUcmpTest(
    const std::vector<NextHopWeight>& swWs,
    const std::vector<NextHopWeight>& hwWs) {
  EXPECT_EQ(swWs.size(), hwWs.size());
  EXPECT_LE(swWs.size(), kNumNextHops);
  auto setup = [this, &swWs, &hwWs]() { programResolvedUcmp(swWs, hwWs); };
  auto verify = [this, &hwWs]() {
    verifyResolvedUcmp(kDefaultRoutePrefix, hwWs);
  };
  verifyAcrossWarmBoots(setup, verify);
}

class HwWideEcmpTest : public HwEcmpTest {
  void SetUp() override {
    FLAGS_ecmp_width = 512;
    HwEcmpTest::SetUp();
  }
};

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
    // necessary because verify() and verifyPostWarmboot() assume that the
    // port is down and the nexthop unresolved, which won't be true if we
    // warmboot after bringing the port up in setupPostWarmboot().
    bringDownPort(nhop.portDesc.phyPortID());
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(HwEcmpTest, ecmpToDropToEcmp) {
  /*
   * Mimic a scenario where all links flap and the routes (including
   * default route) get withdrawn and then peerings comeback one by one
   * and expand the ECMP group
   */
  auto constexpr kEcmpWidthForTest = 4;
  // Program ECMP route
  resolveNeigborAndProgramRoutes(*ecmpHelper_, kEcmpWidthForTest);
  EXPECT_EQ(kEcmpWidthForTest, getEcmpSizeInHw());
  // Mimic neighbor entries going away and route getting removed. Since
  // this is the default route, it actually does not go away but transitions
  // to drop.
  applyNewState(
      ecmpHelper_->unresolveNextHops(getProgrammedState(), kEcmpWidthForTest));
  EXPECT_EQ(0, getEcmpSizeInHw());
  ecmpHelper_->unprogramRoutes(getRouteUpdater());
  // Bring the route back, but mimic learning it from peers one by one first
  applyNewState(
      ecmpHelper_->resolveNextHops(getProgrammedState(), kEcmpWidthForTest));
  for (auto i = 0; i < kEcmpWidthForTest; ++i) {
    ecmpHelper_->programRoutes(getRouteUpdater(), i + 1);
    if (i) {
      EXPECT_EQ(i + 1, getEcmpSizeInHw());
    }
  }
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

TEST_F(HwEcmpTest, UcmpOverflowZero) {
  std::vector<NextHopWeight> swWs, hwWs;
  if (FLAGS_ecmp_width == 64) {
    // default ecmp_width for td2 and tomahawk
    swWs = {50, 50, 1, 1};
    hwWs = {31, 31, 1, 1};
  } else if (FLAGS_ecmp_width == 128) {
    // for tomahawk3
    swWs = {100, 100, 1, 1};
    hwWs = {63, 63, 1, 1};
  } else {
    FAIL()
        << "Do not support ecmp_width other than 64 or 128, please extend the test";
  }
  runSimpleUcmpTest(swWs, hwWs);
}
TEST_F(HwEcmpTest, UcmpOverflowZeroNotEnoughToRoundUp) {
  std::vector<NextHopWeight> swWs, hwWs;
  if (FLAGS_ecmp_width == 64) {
    // default ecmp_width for td2 and tomahawk
    swWs = {50, 50, 1, 1, 1, 1, 1, 1};
    hwWs = {29, 29, 1, 1, 1, 1, 1, 1};
  } else if (FLAGS_ecmp_width == 128) {
    // for tomahawk3
    swWs = {100, 100, 1, 1, 1, 1, 1, 1};
    hwWs = {61, 61, 1, 1, 1, 1, 1, 1};
  } else {
    FAIL()
        << "Do not support ecmp_width other than 64 or 128, please extend the test";
  }
  runSimpleUcmpTest(swWs, hwWs);
}

TEST_F(HwEcmpTest, UcmpRoutesWithSameNextHopsDifferentWeights) {
  std::vector<NextHopWeight> swWs, hwWs;

  if (FLAGS_ecmp_width == 64) {
    // default ecmp_width for td2 and tomahawk
    swWs = {50, 50, 1, 1};
    hwWs = {31, 31, 1, 1};
  } else if (FLAGS_ecmp_width == 128) {
    // for tomahawk3
    swWs = {100, 100, 1, 1};
    hwWs = {63, 63, 1, 1};
  } else {
    FAIL()
        << "Do not support ecmp_width other than 64 or 128, please extend the test";
  }

  std::vector<NextHopWeight> swWs2 = {8, 8, 8, 40};
  const std::vector<NextHopWeight>& hwWs2 = {1, 1, 1, 5};

  EXPECT_EQ(swWs.size(), hwWs.size());
  EXPECT_LE(swWs.size(), kNumNextHops);
  auto setup = [this, &swWs, &swWs2]() {
    auto nHops = swWs.size();
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        nHops,
        {kDefaultRoute},
        std::vector<NextHopWeight>(swWs.begin(), swWs.begin() + nHops));

    auto nHops2 = swWs2.size();
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        nHops2,
        {kRoute2},
        std::vector<NextHopWeight>(swWs2.begin(), swWs2.begin() + nHops2));

    resolveNhops(swWs.size());
  };

  auto verify = [this, &hwWs, &hwWs2]() {
    verifyResolvedUcmp(kDefaultRoutePrefix, hwWs);
    verifyResolvedUcmp(kRoute2Prefix, hwWs2);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Wide UCMP underflow test for when total UCMP weight of the group is less
// than FLAGS_ecmp_width
TEST_F(HwWideEcmpTest, WideUcmpUnderflow) {
  const int numSpineNhops = 5;
  const int numMeshNhops = 3;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

  // skip unsupported platforms
  if (!getHwSwitch()->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WIDE_ECMP)) {
    return;
  }

  auto fillNhops = [](auto& nhops, auto& countAndWeights) {
    auto idx = 0;
    for (const auto& nhopAndWeight : countAndWeights) {
      std::fill(
          nhops.begin() + idx,
          nhops.begin() + idx + nhopAndWeight.first,
          nhopAndWeight.second);
      idx += nhopAndWeight.first;
    }
  };
  // 5 spine nhops of weight 31 and 3 mesh nhops of weight 1
  const std::vector<std::pair<int, int>> nhopsAndWeightsOriginal = {
      {5, 31}, {3, 1}};
  fillNhops(nhops, nhopsAndWeightsOriginal);
  // normalized weights manually computed to compare against
  const std::vector<std::pair<int, int>> nhopsAndWeightsNormalized = {
      {3, 101}, {2, 100}, {3, 3}};
  fillNhops(normalizedNhops, nhopsAndWeightsNormalized);
  runSimpleUcmpTest(nhops, normalizedNhops);
}

TEST_F(HwWideEcmpTest, WideUcmp256WidthUnderflow) {
  const int numSpineNhops = 5;
  const int numMeshNhops = 3;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

  // skip unsupported platforms
  if (!getHwSwitch()->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WIDE_ECMP)) {
    return;
  }

  FLAGS_ecmp_width = 256;
  auto fillNhops = [](auto& nhops, auto& countAndWeights) {
    auto idx = 0;
    for (const auto& nhopAndWeight : countAndWeights) {
      std::fill(
          nhops.begin() + idx,
          nhops.begin() + idx + nhopAndWeight.first,
          nhopAndWeight.second);
      idx += nhopAndWeight.first;
    }
  };
  // 5 spine nhops of weight 31 and 3 mesh nhops of weight 1
  const std::vector<std::pair<int, int>> nhopsAndWeightsOriginal = {
      {5, 31}, {3, 1}};
  fillNhops(nhops, nhopsAndWeightsOriginal);
  // normalized weights manually computed to compare against
  const std::vector<std::pair<int, int>> nhopsAndWeightsNormalized = {
      {3, 51}, {2, 50}, {3, 1}};
  fillNhops(normalizedNhops, nhopsAndWeightsNormalized);
  runSimpleUcmpTest(nhops, normalizedNhops);
}
TEST_F(HwWideEcmpTest, WideUcmpCheckMultipleSlotUnderflow) {
  const int numSpineNhops = 4;
  const int numMeshNhops = 4;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

  // skip unsupported platforms
  if (!getHwSwitch()->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WIDE_ECMP)) {
    return;
  }

  auto fillNhops = [](auto& nhops, auto& countAndWeights) {
    auto idx = 0;
    for (const auto& nhopAndWeight : countAndWeights) {
      std::fill(
          nhops.begin() + idx,
          nhops.begin() + idx + nhopAndWeight.first,
          nhopAndWeight.second);
      idx += nhopAndWeight.first;
    }
  };
  // normalization should happen for two highest weight nexthops
  // since the size of highest weight nexthop group(1) is less than underflow
  const std::vector<std::pair<int, int>> nhopsAndWeightsOriginal = {
      {1, 200}, {4, 50}, {1, 20}, {2, 1}};
  fillNhops(nhops, nhopsAndWeightsOriginal);
  const std::vector<std::pair<int, int>> nhopsAndWeightsNormalized = {
      {1, 242}, {4, 61}, {1, 24}, {2, 1}};
  fillNhops(normalizedNhops, nhopsAndWeightsNormalized);
  runSimpleUcmpTest(nhops, normalizedNhops);
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
      auto ntable = getNdpTable(port, state0);
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
      auto ntable = getNdpTable(port, state1);
      auto entry = entries[port];
      ntable->updateEntry(NeighborEntryFields<folly::IPAddressV6>::fromThrift(
          entry->toThrift()));
    }
    applyNewState(state1);
    ecmpHelper_->programRoutes(getRouteUpdater(), 2);
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

  auto pathsInHw0 = utility::getEcmpMembersInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  auto totalWeightInHw0 =
      utility::getTotalEcmpMemberWeight(getHwSwitch(), pathsInHw0);
  EXPECT_EQ(7, totalWeightInHw0);

  bringUpPort(nhop.portDesc.phyPortID());
  auto pathsInHw1 = utility::getEcmpMembersInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  auto totalWeightInHw1 =
      utility::getTotalEcmpMemberWeight(getHwSwitch(), pathsInHw1);
  EXPECT_EQ(7, totalWeightInHw1);

  unresolveNhops(1);
  resolveNhops(1);
  auto pathsInHw2 = utility::getEcmpMembersInHw(
      getHwSwitch(), kDefaultRoutePrefix, kRid, FLAGS_ecmp_width);
  auto totalWeightInHw2 =
      utility::getTotalEcmpMemberWeight(getHwSwitch(), pathsInHw2);
  EXPECT_EQ(10, totalWeightInHw2);
}
} // namespace facebook::fboss
