// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <fmt/format.h>

#include <utility>

namespace facebook::fboss {

class AgentMySidAdjFrrRouteTest : public AgentHwTest {
 protected:
  static constexpr int kNumLags{4};
  static constexpr uint8_t kMySidPrefixLen{48};
  static constexpr auto kLocatorPrefix{"fdad:ffff::/32"};
  static constexpr auto kSrv6TunnelId{"srv6Tunnel0"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::SRV6_MIDPOINT,
        ProductionFeature::SRV6_ENCAP,
        ProductionFeature::MYSID_ADJACENCY_FRR,
        ProductionFeature::LAG};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /* interfaceHasSubnet */);
    for (int i = 0; i < kNumLags; ++i) {
      utility::addAggPort(
          i + 1,
          {static_cast<int32_t>(ensemble.masterLogicalPortIds()[i])},
          &config);
    }
    config.loadBalancers() =
        utility::getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            ensemble.getL3Asics());
    config.mySidConfig() = makeAdjacencyMySidConfig();
    config.srv6Tunnels() = {utility::makeSrv6TunnelConfig(
        kSrv6TunnelId, InterfaceID(config.interfaces()[0].intfID().value()))};
    return config;
  }

  void SetUp() override {
    AgentHwTest::SetUp();
    if (FLAGS_list_production_feature) {
      return;
    }

    applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");

    auto ecmpHelper = makeEcmpHelper();
    boost::container::flat_set<PortDescriptor> lagPortDescs;
    for (int i = 0; i < kNumLags; ++i) {
      lagPortDescs.emplace(lagPortDesc(i));
    }
    applyNewState(
        [&ecmpHelper,
         &lagPortDescs](const std::shared_ptr<SwitchState>& state) {
          return ecmpHelper.resolveNextHops(
              state, lagPortDescs, true /* useLinkLocal */);
        },
        "resolve adjacency mysid neighbors");

    for (int i = 0; i < kNumLags; ++i) {
      utility::waitForMySidResolveOrUnresolve(
          [this]() { return getProgrammedState(); },
          mySidPrefix(i),
          kMySidPrefixLen,
          true /* resolved */);
    }
  }

  cfg::MySidConfig makeAdjacencyMySidConfig() const {
    cfg::MySidConfig mySidConfig;
    mySidConfig.locatorPrefix() = kLocatorPrefix;
    for (int i = 0; i < kNumLags; ++i) {
      cfg::MySidEntryConfig entry;
      cfg::AdjacencyMySidConfig adjacency;
      adjacency.portName() = fmt::format("AGG-{}", i + 1);
      adjacency.isV6() = true;
      entry.adjacency() = std::move(adjacency);
      mySidConfig.entries()[i + 1] = std::move(entry);
    }
    return mySidConfig;
  }

  utility::EcmpSetupTargetedPorts<folly::IPAddressV6> makeEcmpHelper() {
    return utility::EcmpSetupTargetedPorts<folly::IPAddressV6>(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
  }

  PortDescriptor lagPortDesc(int index) const {
    return PortDescriptor(AggregatePortID(index + 1));
  }

  folly::IPAddressV6 mySidPrefix(int index) const {
    return folly::IPAddressV6(fmt::format("fdad:ffff:{:x}::", index + 1));
  }

  folly::IPAddressV6 backupNextHopAddress(int index) const {
    return folly::IPAddressV6(fmt::format("fdad::{:x}:1", index + 1));
  }

  folly::IPAddressV6 repairSid(int index) const {
    return folly::IPAddressV6(fmt::format("3001:db8:{:x}::", index + 1));
  }

  void programBackupRoutes() {
    auto ecmpHelper = makeEcmpHelper();
    auto routeUpdater = getSw()->getRouteUpdater();
    for (int i = 1; i < kNumLags; ++i) {
      const auto nextHop = ecmpHelper.nhop(lagPortDesc(i));
      const auto nextHopIp = nextHop.linkLocalNhopIp.has_value()
          ? folly::IPAddress(*nextHop.linkLocalNhopIp)
          : folly::IPAddress(nextHop.ip);
      routeUpdater.addRoute(
          RouterID(0),
          backupNextHopAddress(i),
          backupNextHopAddress(i).bitCount(),
          ClientID::OPENR,
          RouteNextHopEntry(
              RouteNextHopSet{
                  ResolvedNextHop(nextHopIp, nextHop.intf, ECMP_WEIGHT)},
              AdminDistance::OPENR));
    }
    routeUpdater.program();
  }

  void addSrv6BackupProtection() {
    programBackupRoutes();

    auto protectedObject = std::make_unique<FrrProtectedObject>();
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() = facebook::network::toBinaryAddress(mySidPrefix(0));
    prefix.prefixLength() = kMySidPrefixLen;
    protectedObject->mySid() = std::move(prefix);

    auto backupNextHops = std::make_unique<std::vector<NextHopThrift>>();
    for (int i = 1; i < kNumLags; ++i) {
      backupNextHops->push_back(
          utility::makeSrv6NextHopThrift(
              backupNextHopAddress(i), repairSid(i), kSrv6TunnelId));
    }

    ThriftHandler(getSw()).addAdjacencyFrr(
        std::move(protectedObject), std::move(backupNextHops));
    waitForBackupResolution();
  }

  void waitForBackupResolution() {
    const auto sidKey = fmt::format(
        "{}/{}", mySidPrefix(0).str(), static_cast<int>(kMySidPrefixLen));
    WITH_RETRIES({
      std::shared_ptr<MySid> mySid;
      for (const auto& [_, mySidMap] :
           std::as_const(*getProgrammedState()->getMySids())) {
        if (auto node = mySidMap->getNodeIf(sidKey)) {
          mySid = node;
          break;
        }
      }
      ASSERT_EVENTUALLY_NE(mySid, nullptr);
      EXPECT_EVENTUALLY_TRUE(mySid->getBackupUnresolveNextHopsId().has_value());
      const auto resolvedId = mySid->getBackupResolvedNextHopsId();
      ASSERT_EVENTUALLY_TRUE(resolvedId.has_value());
      const auto manager = getSw()->getRib()->getNextHopIDManagerCopy();
      ASSERT_EVENTUALLY_NE(manager, nullptr);
      const auto resolvedNextHops = manager->getNextHopsIf(*resolvedId);
      ASSERT_EVENTUALLY_TRUE(resolvedNextHops.has_value());
      EXPECT_EVENTUALLY_EQ(resolvedNextHops->size(), kNumLags - 1);
      for (const auto& nextHop : *resolvedNextHops) {
        EXPECT_EQ(nextHop.role(), NextHopRole::BACKUP);
        EXPECT_EQ(nextHop.tunnelType(), TunnelType::SRV6_ENCAP);
        EXPECT_FALSE(nextHop.srv6SegmentList().empty());
      }
    });
  }
};

TEST_F(AgentMySidAdjFrrRouteTest, addSrv6BackupProtection) {
  auto setup = [this]() { addSrv6BackupProtection(); };
  auto verify = []() {};
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
