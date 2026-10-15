/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/MatchAction.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

DECLARE_bool(enable_nexthop_id_manager);

namespace facebook::fboss {

namespace {
constexpr uint8_t kDscp = 10;
} // namespace

class AgentPbrTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::PBR_ACL};
  }

 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addAclTable(
        &cfg,
        cfg::switch_config_constants::DEFAULT_PBR_ACL_TABLE(),
        1 /*priority*/,
        {cfg::AclTableActionType::COUNTER, cfg::AclTableActionType::REDIRECT},
        {cfg::AclTableQualifier::TC,
         cfg::AclTableQualifier::NEXT_HOP_GROUP_ID});
    utility::addQosMapsHelper(
        cfg,
        {{1 /*TC==CLASS_1*/, {kDscp}}},
        "pbrQos",
        ensemble.getSw()->getHwAsicTable()->getL3Asics());
    return cfg;
  }

  utility::EcmpSetupTargetedPorts6 ecmpHelper() const {
    return utility::EcmpSetupTargetedPorts6(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
  }

  RouteNextHopSet makeNamedNhgSet(
      const std::vector<PortDescriptor>& ports) const {
    auto helper = ecmpHelper();
    RouteNextHopSet nhops;
    for (const auto& port : ports) {
      nhops.insert(UnresolvedNextHop(
          folly::IPAddress(helper.nhop(port).ip), ECMP_WEIGHT));
    }
    return nhops;
  }

  void addOrUpdateNamedNhgs(
      const std::vector<std::pair<std::string, RouteNextHopSet>>& groups) {
    auto nhgs = std::make_unique<std::vector<NextHopGroup>>();
    for (const auto& [name, nhops] : groups) {
      NextHopGroup group;
      group.name() = name;
      group.nexthops() = {};
      for (const auto& nhop : nhops) {
        group.nexthops()->push_back(nhop.toThrift());
      }
      nhgs->push_back(std::move(group));
    }
    ThriftHandler handler(getSw());
    handler.addOrUpdateNamedNextHopGroups(std::move(nhgs));
  }

  NextHopGroup makeLinkLocalNhgGroup(
      const std::string& name,
      const std::vector<PortDescriptor>& ports) const {
    NextHopGroup group;
    group.name() = name;
    group.nexthops() = {};
    auto helper = ecmpHelper();
    for (const auto& port : ports) {
      auto ecmpNhop = helper.nhop(port);
      CHECK(ecmpNhop.linkLocalNhopIp.has_value());
      NextHop nh = ResolvedNextHop(
          folly::IPAddress(*ecmpNhop.linkLocalNhopIp),
          ecmpNhop.intf,
          ECMP_WEIGHT);
      group.nexthops()->push_back(nh.toThrift());
    }
    return group;
  }

  std::string kDefaultNhg() const {
    return "pbrDefaultNhg";
  }
  std::string kClassNhg() const {
    return "pbrClassNhg";
  }
  std::string kPolicy() const {
    return "pbrPolicy";
  }

  void verifyRedirectAndCounter() {
    auto state = getProgrammedState();
    auto aclMap = state->getAclsForTable(
        cfg::AclStage::INGRESS,
        cfg::switch_config_constants::DEFAULT_PBR_ACL_TABLE());
    ASSERT_NE(aclMap, nullptr);
    int pbrEntries = 0;
    int entriesWithCounter = 0;
    std::optional<std::string> counterName;
    for (const auto& [_, entry] : std::as_const(*aclMap)) {
      if (entry->getNextHopGroupId().has_value()) {
        ++pbrEntries;
        if (auto aclAction = entry->getAclAction()) {
          auto matchAction = MatchAction::fromThrift(aclAction->toThrift());
          if (auto trafficCounter = matchAction.getTrafficCounter()) {
            ++entriesWithCounter;
            counterName = *trafficCounter->name();
          }
        }
      }
    }
    // The fixture programs one policy with one traffic class, so a second
    // entry means a generation was retired without removing its entry.
    EXPECT_EQ(pbrEntries, 1);

    if (!isSupportedOnAllAsics(HwAsic::Feature::PBR_ACL)) {
      return;
    }
    EXPECT_EQ(entriesWithCounter, pbrEntries);
    ASSERT_TRUE(counterName.has_value());
  }
};

} // namespace facebook::fboss
