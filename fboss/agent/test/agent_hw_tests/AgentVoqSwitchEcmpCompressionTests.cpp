// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchFullScaleDsfTests.h"
#include "fboss/agent/test/utils/EcmpResourceManagerTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

DECLARE_bool(list_production_feature);

namespace facebook::fboss {

class AgentVoqSwitchEcmpCompressionTest
    : public AgentVoqSwitchFullScaleDsfNodesTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchFullScaleDsfNodesTest::initialConfig(ensemble);
    cfg.switchSettings()->ecmpCompressionThresholdPct() = 100;
    return cfg;
  }
  void SetUp() override;
  void TearDown() override;

  void setCmdLineFlagOverrides() const override {
    AgentVoqSwitchFullScaleDsfNodesTest::setCmdLineFlagOverrides();
    FLAGS_enable_ecmp_resource_manager = true;
  }

  int numStartRoutes() const {
    return getMaxEcmpGroup() -
        FLAGS_ecmp_resource_manager_make_before_break_buffer;
  }
  const EcmpResourceManager* ecmpResourceManager() const {
    return getSw()->getEcmpResourceManager();
  }

  RoutePrefixV6 makePrefix(int index) const {
    return RoutePrefixV6{
        folly::IPAddressV6(folly::sformat("2401:db00:23{}::", index + 1)), 48};
  }
  boost::container::flat_set<PortDescriptor> getNextHops(int index) const {
    auto kNhopOffset = 4;
    CHECK_LT(index * kNhopOffset + getMaxEcmpWidth(), sysPortDescs_.size());
    auto sysPortStart = (index * kNhopOffset);
    return boost::container::flat_set<PortDescriptor>(
        std::make_move_iterator(sysPortDescs_.begin() + sysPortStart),
        std::make_move_iterator(sysPortDescs_.begin() + getMaxEcmpWidth()));
  }

  std::set<EcmpResourceManager::Prefix> getPrefixesForGroups(
      const EcmpResourceManager::NextHopGroupIds& groups) const {
    auto gidToPrefixes = ecmpResourceManager()->getGroupIdToPrefix();
    std::set<EcmpResourceManager::Prefix> prefixes;
    for (const auto& [gid, pfxs] :
         ecmpResourceManager()->getGroupIdToPrefix()) {
      if (groups.contains(gid)) {
        prefixes.insert(pfxs.begin(), pfxs.end());
      }
    }
    return prefixes;
  }

 private:
  boost::container::flat_set<PortDescriptor> sysPortDescs_;
};

void AgentVoqSwitchEcmpCompressionTest::SetUp() {
  XLOG(DBG2) << " SetUp start";
  AgentVoqSwitchFullScaleDsfNodesTest::SetUp();
  if (!getAgentEnsemble()) {
    CHECK(FLAGS_list_production_feature);
    return;
  }
  utility::setupRemoteIntfAndSysPorts(
      getSw(),
      isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());

  // Resolve remote nhops and get a list of remote sysPort descriptors
  sysPortDescs_ = utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);

  std::vector<RoutePrefixV6> prefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> nhops;
  for (int i = 0; i < numStartRoutes(); i++) {
    prefixes.emplace_back(makePrefix(i));
    nhops.emplace_back(getNextHops(i));
  }
  auto routeUpdater = getSw()->getRouteUpdater();
  XLOG(DBG2) << " Programming starting routes";
  ecmpHelper.programRoutes(&routeUpdater, nhops, prefixes);
  XLOG(DBG2) << " Programming starting routes done";
}

void AgentVoqSwitchEcmpCompressionTest::TearDown() {
  if (!getAgentEnsemble()) {
    CHECK(FLAGS_list_production_feature);
    return;
  }
  assertResourceMgrCorrectness(*ecmpResourceManager(), getProgrammedState());
  AgentVoqSwitchFullScaleDsfNodesTest::TearDown();
}

TEST_F(AgentVoqSwitchEcmpCompressionTest, addOneRouteOverEcmpLimit) {
  auto setup = [&]() {
    auto optimalMergeSet = ecmpResourceManager()->getOptimalMergeGroupSet();
    EXPECT_EQ(optimalMergeSet.size(), 2);
    auto toBeMergedPrefixes = getPrefixesForGroups(optimalMergeSet);
    EXPECT_EQ(toBeMergedPrefixes.size(), 2);
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    XLOG(DBG2) << " Adding overflow route : "
               << makePrefix(numStartRoutes()).str();
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        getNextHops(numStartRoutes()),
        {makePrefix(numStartRoutes())});
    for (const auto& ridAndPfx : toBeMergedPrefixes) {
      auto [rid, pfx] = ridAndPfx;
      auto groupInfo = ecmpResourceManager()->getGroupInfo(rid, pfx);
      ASSERT_TRUE(groupInfo->hasOverrideNextHops());
      XLOG(DBG2) << " Prefix : " << ridAndPfx << " points to merged group: "
                 << folly::join(
                        ", ", (*groupInfo->getMergedGroupInfoItr())->first);
    }
  };
  auto verify = [&]() {
    assertNumRoutesWithNhopOverrides(getProgrammedState(), 2);
    EXPECT_EQ(ecmpResourceManager()->getMergedGroups().size(), 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
