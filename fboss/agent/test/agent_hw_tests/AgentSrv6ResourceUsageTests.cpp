// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AddressUtil.h"

namespace facebook::fboss {

class AgentSrv6ResourceUsageTest : public AgentHwTest {
 protected:
  const folly::IPAddressV6 kDecapMySidAddr{"3001:db8:efff::"};
  static constexpr uint8_t kDecapMySidPrefixLen{48};

  // SID offset for the adjacency (uA) mySid: 3001:db8:1::/48, kept clear of the
  // decap SID above. Matches utility::makeAdjacencyMySidEntries' SID layout.
  static constexpr int kAdjSidOffset{1};

  // Each mySid entry (decap or adjacency) consumes one HW slot.
  static constexpr int kMySidEntryCost{1};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::SRV6_DECAP, ProductionFeature::SRV6_MIDPOINT};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  utility::EcmpSetupAnyNPorts<folly::IPAddressV6> makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
  }

  void removeDecapMySidEntry() {
    IpPrefix ipPrefix;
    ipPrefix.ip() = facebook::network::toBinaryAddress(kDecapMySidAddr);
    ipPrefix.prefixLength() = kDecapMySidPrefixLen;
    auto sw = getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        std::vector<MySidEntry>{} /* toAdd */,
        {ipPrefix},
        "removeDecapMySidEntry",
        ribMySidToSwitchStateFunc,
        sw);
  }

  int32_t getMySidEntriesFree() {
    auto switchId = getCurrentSwitchIdForTesting();
    getLatestPortStats(masterLogicalPortIds());
    auto stats = getSw()->getHwSwitchStatsExpensive(switchId);
    return stats.hwResourceStats()->my_sid_entries_free().value();
  }
};

TEST_F(AgentSrv6ResourceUsageTest, verifyMySidResourceUsage) {
  auto setup = []() {};

  auto verify = [this]() {
    auto mySidFreeBefore = getMySidEntriesFree();

    // Add decap mySid, verify counter drops by one entry.
    utility::addDecapMySidEntry(getSw(), kDecapMySidAddr, kDecapMySidPrefixLen);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(), mySidFreeBefore - kMySidEntryCost);
    });

    // Add adjacency mySid (interface baked into the resolved next hop so the
    // unresolved/resolved next-hop-set IDs coincide), verify counter drops by
    // another entry.
    auto ecmpHelper = makeEcmpHelper();
    resolveNeighbors(ecmpHelper, 1);
    utility::programMySidEntries(
        getSw(),
        utility::makeAdjacencyMySidEntries(
            ecmpHelper, 1 /*numNhops*/, 1 /*numEntries*/, kAdjSidOffset));
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(), mySidFreeBefore - 2 * kMySidEntryCost);
    });

    // Remove adjacency mySid, verify counter returns to the post-decap level.
    utility::deleteScaleMySidEntries(getSw(), 1 /*numEntries*/, kAdjSidOffset);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(), mySidFreeBefore - kMySidEntryCost);
    });

    // Remove decap mySid, verify counter returns to baseline.
    removeDecapMySidEntry();
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
