/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fmt/core.h>
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
#include "fboss/agent/test/utils/Srv6TestUtils.h"

DECLARE_bool(enable_nexthop_id_manager);

namespace facebook::fboss {

namespace {
constexpr uint8_t kDscp = 10;
constexpr auto kSrv6TunnelId = "srv6Tunnel0";
constexpr uint16_t kL4SrcPort = 10000;
constexpr uint16_t kL4DstPort = 20000;
constexpr int kNumPkts = 10;
constexpr uint64_t kVariance = 2;
constexpr auto kPbrDstIp = "2222::1";
constexpr auto kPbrSrcIp = "2001::1";

// Keyed off the physical port so a next hop keeps its SID across warm boot and
// across the member-change sequence.
std::vector<folly::IPAddressV6> makeSidList(PortID port) {
  auto index = static_cast<int>(port);
  return {folly::IPAddressV6(
      fmt::format("3001:db8:{:x}:{:x}::", (index >> 8) + 1, index & 0xFF))};
}

template <typename NextHopT, typename... Args>
NextHopT makeSrv6NextHop(PortID port, Args&&... args) {
  return NextHopT(
      std::forward<Args>(args)...,
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      makeSidList(port),
      TunnelType::SRV6_ENCAP,
      std::string(kSrv6TunnelId));
}
} // namespace

class AgentPbrTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::PBR_ACL, ProductionFeature::SRV6_ENCAP};
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
    cfg.srv6Tunnels() = {utility::makeSrv6TunnelConfig(
        kSrv6TunnelId, InterfaceID(cfg.interfaces()[0].intfID().value()))};
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
      nhops.insert(
          makeSrv6NextHop<UnresolvedNextHop>(
              port.phyPortID(), folly::IPAddress(helper.nhop(port).ip)));
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
      NextHop nh = makeSrv6NextHop<ResolvedNextHop>(
          port.phyPortID(),
          folly::IPAddress(*ecmpNhop.linkLocalNhopIp),
          ecmpNhop.intf);
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

  void verifyRedirectAndCounter(
      const std::vector<PortID>& classMembers,
      const std::vector<PortID>& defaultMembers,
      PortID injectPort) {
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

    auto intfMac = getMacForFirstInterfaceWithPortsForTesting(state);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto sumOut = [&](const std::vector<PortID>& members) {
      uint64_t sum = 0;
      for (const auto& port : members) {
        sum += *getNextUpdatedPortStats(port).outUnicastPkts_();
      }
      return sum;
    };

    // A counter whose entry was just programmed reports a garbage value on its
    // first stats poll, so only trust a baseline two successive ticks agree on.
    uint64_t pktsBefore = utility::getAclInOutPackets(getSw(), *counterName);
    WITH_RETRIES({
      getNextUpdatedPortStats(injectPort);
      auto pkts = utility::getAclInOutPackets(getSw(), *counterName);
      EXPECT_EVENTUALLY_EQ(pkts, pktsBefore);
      pktsBefore = pkts;
    });
    auto classBefore = sumOut(classMembers);
    auto defaultBefore = sumOut(defaultMembers);
    for (int i = 0; i < kNumPkts; ++i) {
      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          getVlanIDForTx(),
          srcMac,
          intfMac,
          folly::IPAddressV6(kPbrSrcIp),
          folly::IPAddressV6(kPbrDstIp),
          kL4SrcPort + i, // vary L4 src to spread across the class NHG members
          kL4DstPort,
          kDscp << 2 /*trafficClass byte*/,
          255 /*hopLimit*/);
      getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), injectPort);
    }

    WITH_RETRIES({
      EXPECT_EVENTUALLY_GE(sumOut(classMembers) - classBefore, kNumPkts);
      // A redirect that only partly works leaks probe packets here in bulk,
      // so the bound has to stay well under kNumPkts. It is not zero because
      // control-plane traffic also egresses these ports.
      EXPECT_EVENTUALLY_LE(sumOut(defaultMembers) - defaultBefore, kVariance);
      auto pktsAfter = utility::getAclInOutPackets(getSw(), *counterName);
      EXPECT_EVENTUALLY_GE(pktsAfter, pktsBefore + kNumPkts);
    });
  }
};

} // namespace facebook::fboss
