// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AddressUtil.h"

namespace facebook::fboss {

class AgentSrv6ResourceUsageTest : public AgentHwTest {
 protected:
  const folly::IPAddressV6 kDecapMySidAddr{"3001:db8:efff::"};
  static constexpr uint8_t kDecapMySidPrefixLen{48};

  const folly::IPAddressV6 kMidpointMySidPrefix{"3001:db8:1::"};
  static constexpr uint8_t kMidpointMySidPrefixLen{48};

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

  void addDecapMySidEntry() {
    MySidEntry entry;
    entry.type() = MySidType::DECAPSULATE_AND_LOOKUP;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(kDecapMySidAddr);
    prefix.prefixLength() = kDecapMySidPrefixLen;
    entry.mySid() = prefix;
    auto sw = getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addDecapMySidEntry",
        ribMySidToSwitchStateFunc,
        sw);
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

  void addAdjacencyMySidEntry(
      const folly::IPAddress& nexthopIp,
      InterfaceID nexthopIntf) {
    state::MySidFields fields;
    fields.type() = MySidType::ADJACENCY_MICRO_SID;
    fields.adjacencyInterfaceId() = static_cast<int32_t>(nexthopIntf);
    fields.isV6() = nexthopIp.isV6();
    fields.clientId() = ClientID::STATIC_ROUTE;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() = facebook::network::toBinaryAddress(
        folly::IPAddress(kMidpointMySidPrefix));
    prefix.prefixLength() = kMidpointMySidPrefixLen;
    fields.mySid() = prefix;
    auto mySid = std::make_shared<MySid>(fields);
    RouteNextHopSet nhops{ResolvedNextHop(nexthopIp, nexthopIntf, ECMP_WEIGHT)};
    std::vector<MySidWithNextHops> toAdd = {{mySid, nhops}};

    auto sw = getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        std::move(toAdd),
        {} /* toUnresolveIfMatch */,
        {} /* toDelete */,
        "addAdjacencyMySidEntry",
        ribMySidToSwitchStateFunc,
        sw);
  }

  void removeAdjacencyMySidEntry() {
    IpPrefix ipPrefix;
    ipPrefix.ip() = facebook::network::toBinaryAddress(
        folly::IPAddress(kMidpointMySidPrefix));
    ipPrefix.prefixLength() = kMidpointMySidPrefixLen;
    auto sw = getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        std::vector<MySidEntry>{} /* toAdd */,
        {ipPrefix},
        "removeAdjacencyMySidEntry",
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

    // Add decap mySid, verify counter decreases by 1.
    addDecapMySidEntry();
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore - 1); });

    // Add adjacency mySid, verify counter decreases by 2 total.
    auto ecmpHelper = makeEcmpHelper();
    applyNewState([&ecmpHelper](std::shared_ptr<SwitchState> in) {
      return ecmpHelper.resolveNextHops(in, 1);
    });
    const auto ecmpNhop = ecmpHelper.nhop(0);
    const folly::IPAddress nexthopIp = folly::IPAddress(ecmpNhop.ip);
    addAdjacencyMySidEntry(nexthopIp, ecmpNhop.intf);
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore - 2); });

    // Remove adjacency mySid, verify counter goes back to -1.
    removeAdjacencyMySidEntry();
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore - 1); });

    // Remove decap mySid, verify counter returns to baseline.
    removeDecapMySidEntry();
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getMySidEntriesFree(), mySidFreeBefore); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
