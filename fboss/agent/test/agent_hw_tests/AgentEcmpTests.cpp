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

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/EcmpTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(intf_nbr_tables);

namespace {
facebook::fboss::RoutePrefixV6 kDefaultRoute{folly::IPAddressV6(), 0};
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};

facebook::fboss::RoutePrefixV6 kRoute2{
    folly::IPAddressV6{"2803:6080:d038:3063::"},
    64};
folly::CIDRNetwork kRoute2Prefix{folly::IPAddress("2803:6080:d038:3063::"), 64};
const auto numNeighborEntries = 2;
} // namespace
namespace facebook::fboss {

class AgentEcmpTest : public AgentHwTest {
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
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

  void resolveNhops(int numNhops) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, numNhops);
    });
  }
  void resolveNhops(const std::vector<PortDescriptor>& portDescs) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(
          in, flat_set<PortDescriptor>(portDescs.begin(), portDescs.end()));
    });
  }
  void unresolveNhops(int numNhops) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.unresolveNextHops(in, numNhops);
    });
  }
  void unresolveNhops(const std::vector<PortDescriptor>& portDescs) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.unresolveNextHops(
          in, flat_set<PortDescriptor>(portDescs.begin(), portDescs.end()));
    });
  }
  void programRouteWithUnresolvedNhops(int numNhops = kNumNextHops) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &wrapper,
        numNhops,
        {kDefaultRoute},
        std::vector<NextHopWeight>(
            swSwitchWeights_.begin(), swSwitchWeights_.begin() + numNhops));
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

 protected:
  int getEcmpSizeInHw() const {
    facebook::fboss::utility::CIDRNetwork cidr;
    cidr.IPAddress() = "::";
    cidr.mask() = 0;
    return utility::getEcmpSizeInHw(getAgentEnsemble(), cidr);
  }
};

void AgentEcmpTest::programResolvedUcmp(
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

void AgentEcmpTest::verifyResolvedUcmp(
    const folly::CIDRNetwork& routePrefix,
    const std::vector<NextHopWeight>& hwWs) {
  auto switchId =
      getSw()->getScopeResolver()->scope(masterLogicalPortIds()[0]).switchId();
  auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
  facebook::fboss::utility::CIDRNetwork cidr;
  cidr.IPAddress() = routePrefix.first.str();
  cidr.mask() = routePrefix.second;
  WITH_RETRIES({
    std::map<int32_t, int32_t> pathsInHw;
    client->sync_getEcmpWeights(pathsInHw, cidr, kRid);
    auto pathsInHwCount = pathsInHw.size();
    EXPECT_EVENTUALLY_LE(pathsInHwCount, FLAGS_ecmp_width);

    // This check assumes that egress ids grow as you add more egresses
    // That assumption could prove incorrect, in which case we would
    // need to map ips to egresses, somehow.
    auto expectedCountIter = hwWs.begin();
    for (const auto& [member, pathWeight] : pathsInHw) {
      XLOG(DBG2) << "member: " << member << " pathWeight: " << pathWeight;
      EXPECT_EVENTUALLY_EQ(pathWeight, *expectedCountIter);
      expectedCountIter++;
    }
    auto totalHwWeight =
        std::accumulate(hwWs.begin(), hwWs.end(), NextHopWeight(0));
    auto totalWeightInHw = 0;
    for (auto& [_, weight] : pathsInHw) {
      totalWeightInHw += weight;
    }
    EXPECT_EVENTUALLY_EQ(totalHwWeight, totalWeightInHw);
  });
}

void AgentEcmpTest::runSimpleUcmpTest(
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

class AgentEcmpTestWithWBWrites : public AgentEcmpTest {
 public:
  bool failHwCallsOnWarmboot() const override {
    return false;
  }
};

// WB is expected to create next hop/group corresponding to port brought down
TEST_F(AgentEcmpTestWithWBWrites, L2ResolveOneNhopThenLinkDownThenUp) {
  auto setup = [=, this]() {
    programRouteWithUnresolvedNhops();
    resolveNhops(1);

    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(1, getEcmpSizeInHw()); });
  };

  auto verify = [=, this]() {
    // ECMP shrunk on port down
    WITH_RETRIES({
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
      auto nhop = ecmpHelper.nhop(0);
      bringDownPort(nhop.portDesc.phyPortID());
      EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw());
    });
  };

  auto setupPostWarmboot = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
    auto nhop = ecmpHelper.nhop(0);
    bringUpPort(nhop.portDesc.phyPortID());
  };

  auto verifyPostWarmboot = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
    auto nhop = ecmpHelper.nhop(0);
    // ECMP stays shrunk on port up
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw()); });

    // Bring port back down so we can warmboot more than once. This is
    // necessary because verify() and verifyPostWarmboot() assume that the
    // port is down and the nexthop unresolved, which won't be true if we
    // warmboot after bringing the port up in setupPostWarmboot().
    bringDownPort(nhop.portDesc.phyPortID());
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(AgentEcmpTest, ecmpToDropToEcmp) {
  /*
   * Mimic a scenario where all links flap and the routes (including
   * default route) get withdrawn and then peerings comeback one by one
   * and expand the ECMP group
   */
  auto constexpr kEcmpWidthForTest = 4;
  auto setup = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
    // Program ECMP route
    resolveNeighborAndProgramRoutes(ecmpHelper, kEcmpWidthForTest);
  };
  auto verify = [=, this]() {
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(kEcmpWidthForTest, getEcmpSizeInHw()); });

    // Mimic neighbor entries going away and route getting removed. Since
    // this is the default route, it actually does not go away but transitions
    // to drop.
    unresolveNhops(kEcmpWidthForTest);
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw()); });
    auto wrapper = getSw()->getRouteUpdater();
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
    ecmpHelper.unprogramRoutes(&wrapper);
    // Bring the route back, but mimic learning it from peers one by one first
    resolveNhops(kEcmpWidthForTest);
    for (auto i = 0; i < kEcmpWidthForTest; ++i) {
      ecmpHelper.programRoutes(&wrapper, i + 1);
      if (i) {
        WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(i + 1, getEcmpSizeInHw()); });
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentEcmpTest, L2ResolveOneNhopThenLinkDownThenUpThenL2ResolveNhop) {
  auto setup = [=, this]() {
    programRouteWithUnresolvedNhops();
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto nhop = ecmpHelper.nhop(0);
    resolveNhops(1);
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(1, getEcmpSizeInHw()); });
    bringDownPort(nhop.portDesc.phyPortID());
    // ECMP shrunk on port down
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw()); });
    bringUpPort(nhop.portDesc.phyPortID());
    // ECMP stays shrunk on port up
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw()); });
    // Re resolve nhop1
    unresolveNhops(1);
    resolveNhops(1);
  };

  auto verify = [=, this]() {
    // ECMP  expands post resolution
    EXPECT_EQ(1, getEcmpSizeInHw());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentEcmpTest, L2UnresolvedNhopsECMPInHWEmpty) {
  auto setup = [=, this]() { programRouteWithUnresolvedNhops(); };
  auto verify = [=, this]() {
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(0, getEcmpSizeInHw()); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentWideEcmpTest : public AgentEcmpTest {
  void setCmdLineFlagOverrides() const override {
    FLAGS_ecmp_width = 512;
    FLAGS_wide_ecmp = true;
    AgentHwTest::setCmdLineFlagOverrides();
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING, ProductionFeature::WIDE_ECMP};
  }
};

// Wide UCMP underflow test for when total UCMP weight of the group is less
// than FLAGS_ecmp_width
TEST_F(AgentWideEcmpTest, WideUcmpUnderflow) {
  const int numSpineNhops = 5;
  const int numMeshNhops = 3;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

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

class Agent256WideEcmpTest : public AgentWideEcmpTest {
  void setCmdLineFlagOverrides() const override {
    FLAGS_wide_ecmp = true;
    FLAGS_ecmp_width = 256;
    AgentHwTest::setCmdLineFlagOverrides();
  }
};

TEST_F(Agent256WideEcmpTest, WideUcmp256WidthUnderflow) {
  const int numSpineNhops = 5;
  const int numMeshNhops = 3;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

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
TEST_F(AgentWideEcmpTest, WideUcmpCheckMultipleSlotUnderflow) {
  const int numSpineNhops = 4;
  const int numMeshNhops = 4;
  std::vector<uint64_t> nhops(numSpineNhops + numMeshNhops);
  std::vector<uint64_t> normalizedNhops(numSpineNhops + numMeshNhops);

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

template <bool enableIntfNbrTable>
struct EnableIntfNbrTable {
  static constexpr auto intfNbrTable = enableIntfNbrTable;
};

using NeighborTableTypes =
    ::testing::Types<EnableIntfNbrTable<false>, EnableIntfNbrTable<true>>;

template <typename EnableIntfNbrTableT>
class AgentEcmpNeighborTest : public AgentEcmpTest {
  static auto constexpr intfNbrTable = EnableIntfNbrTableT::intfNbrTable;

  void setCmdLineFlagOverrides() const override {
    FLAGS_intf_nbr_tables = isIntfNbrTable();
    AgentHwTest::setCmdLineFlagOverrides();
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if (intfNbrTable) {
      return {
          ProductionFeature::L3_FORWARDING,
          ProductionFeature::INTERFACE_NEIGHBOR_TABLE};
    } else {
      return {
          ProductionFeature::L3_FORWARDING,
          ProductionFeature::VLAN,
          ProductionFeature::MAC_LEARNING};
    }
  }

 public:
  bool isIntfNbrTable() const {
    return intfNbrTable == true;
  }

  auto getNdpTable(PortDescriptor port, std::shared_ptr<SwitchState>& state) {
    const auto switchType =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getSwitchType();

    if (isIntfNbrTable() || switchType == cfg::SwitchType::VOQ) {
      auto portId = port.phyPortID();
      InterfaceID intfId(
          *state->getPorts()->getNode(portId)->getInterfaceIDs().begin());
      return state->getInterfaces()->getNode(intfId)->getNdpTable()->modify(
          intfId, &state);
    } else if (switchType == cfg::SwitchType::NPU) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), getSw()->needL2EntryForNeighbor());
      auto vlanId = ecmpHelper.getVlan(port, getProgrammedState());
      return state->getVlans()->getNode(*vlanId)->getNdpTable()->modify(
          *vlanId, &state);
    }

    XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
  }
};

TYPED_TEST_SUITE(AgentEcmpNeighborTest, NeighborTableTypes);

TYPED_TEST(AgentEcmpNeighborTest, ResolvePendingResolveNexthop) {
  auto setup = [=, this]() {
    this->resolveNhops(numNeighborEntries);
    std::map<PortDescriptor, std::shared_ptr<NdpEntry>> entries;
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());

    // mark neighbors connected over ports pending
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = in->clone();
      for (auto i = 0; i < numNeighborEntries; i++) {
        const auto& ecmpNextHop = ecmpHelper.nhop(i);
        auto port = ecmpNextHop.portDesc;
        auto ntable = this->getNdpTable(port, state);
        auto entry = ntable->getEntry(ecmpNextHop.ip);
        auto intfId = entry->getIntfID();
        ntable->removeEntry(ecmpNextHop.ip);
        ntable->addPendingEntry(ecmpNextHop.ip, intfId);
        entries[port] = std::move(entry);
      }
      return state;
    });

    // mark neighbors connected over ports reachable
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = in->clone();
      for (auto i = 0; i < numNeighborEntries; i++) {
        const auto& ecmpNextHop = ecmpHelper.nhop(i);
        auto port = ecmpNextHop.portDesc;
        auto ntable = this->getNdpTable(port, state);
        auto entry = entries[port];
        ntable->updateEntry(
            NeighborEntryFields<folly::IPAddressV6>::fromThrift(
                entry->toThrift()));
      }
      return state;
    });
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, 2);
  };
  auto verify = [=, this]() {
    /* ecmp is resolved */
    WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(this->getEcmpSizeInHw(), 2); });
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

class AgentUcmpTest : public AgentEcmpTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING, ProductionFeature::UCMP};
  }
};

TEST_F(AgentUcmpTest, UcmpOverflowZero) {
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

TEST_F(AgentUcmpTest, UcmpOverflowZeroNotEnoughToRoundUp) {
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

TEST_F(AgentUcmpTest, UcmpRoutesWithSameNextHopsDifferentWeights) {
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
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto nHops = swWs.size();
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &wrapper,
        nHops,
        {kDefaultRoute},
        std::vector<NextHopWeight>(swWs.begin(), swWs.begin() + nHops));

    auto nHops2 = swWs2.size();
    ecmpHelper.programRoutes(
        &wrapper,
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

// Test link down in UCMP scenario
TEST_F(AgentUcmpTest, UcmpL2ResolveAllNhopsInThenLinkDown) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  programResolvedUcmp({3, 1, 1, 1, 1, 1, 1, 1}, {3, 1, 1, 1, 1, 1, 1, 1});
  bringDownPort(ecmpHelper.nhop(0).portDesc.phyPortID());
  WITH_RETRIES({
    auto pathsInHwCount = getEcmpSizeInHw();
    EXPECT_EVENTUALLY_EQ(7, pathsInHwCount);
  });
}

// Test link flap in UCMP scenario
TEST_F(AgentUcmpTest, UcmpL2ResolveBothNhopsInThenLinkFlap) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  programResolvedUcmp({3, 1, 1, 1, 1, 1, 1, 1}, {3, 1, 1, 1, 1, 1, 1, 1});
  auto nhop = ecmpHelper.nhop(0);
  bringDownPort(nhop.portDesc.phyPortID());

  WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(7, getEcmpSizeInHw()); });

  bringUpPort(nhop.portDesc.phyPortID());
  WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(7, getEcmpSizeInHw()); });

  unresolveNhops(1);
  resolveNhops(1);
  WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(10, getEcmpSizeInHw()); });
}

} // namespace facebook::fboss
