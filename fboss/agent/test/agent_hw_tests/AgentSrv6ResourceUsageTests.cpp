// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
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

  // Yuba G200 uDT installs two HW endpoints (exact /128 match + prefix miss
  // action).
  static constexpr int32_t kDecapMySidSlotsPerEntry{2};
  // uA mySid consumes one HW slot.
  static constexpr int32_t kAdjacencyMySidSlotsPerEntry{1};

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
    if (mySidFreeBefore == hardware_stats_constants::STAT_UNINITIALIZED()) {
      GTEST_SKIP()
          << "my_sid_entries_free unavailable (STAT_UNINITIALIZED). "
          << "Ensure srv6 is enabled in agent config defaultCommandLineArgs "
          << "and the ASIC supports SRV6_MYSID_RESOURCE_COUNTER.";
    }

    // Add decap (uDT) mySid — consumes 2 slots on Yuba G200.
    utility::addDecapMySidEntry(getSw(), kDecapMySidAddr, kDecapMySidPrefixLen);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(), mySidFreeBefore - kDecapMySidSlotsPerEntry);
    });

    // Add adjacency (uA) mySid — one slot; decap + uA = 3 total.
    auto ecmpHelper = makeEcmpHelper();
    resolveNeighbors(ecmpHelper, 1);
    utility::programMySidEntries(
        getSw(),
        utility::makeAdjacencyMySidEntries(
            ecmpHelper, 1 /*numNhops*/, 1 /*numEntries*/, kAdjSidOffset));
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(),
          mySidFreeBefore - kDecapMySidSlotsPerEntry -
              kAdjacencyMySidSlotsPerEntry);
    });

    // Remove adjacency mySid — only decap (2 slots) remain.
    utility::deleteScaleMySidEntries(getSw(), 1 /*numEntries*/, kAdjSidOffset);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getMySidEntriesFree(), mySidFreeBefore - kDecapMySidSlotsPerEntry);
    });

    // Remove decap mySid, verify counter returns to baseline.
    removeDecapMySidEntry();
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
