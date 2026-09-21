// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
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

  // uSID packet aimed at the protected SID: [locator fdad:ffff:]
  // [active uSID 1][next uSID f]. Function id 0xf is outside the configured
  // 1..kNumLags, so once uSID 1 is shifted out the packet is not another
  // local SID on this box.
  folly::IPAddressV6 protectedSidPktDst() const {
    return folly::IPAddressV6("fdad:ffff:1:f::");
  }

  PortID getEgressPort(const PortDescriptor& portDesc) const {
    if (portDesc.isPhysicalPort()) {
      return portDesc.phyPortID();
    }
    auto aggPort = getProgrammedState()->getAggregatePorts()->getNodeIf(
        portDesc.aggPortID());
    return aggPort->sortedSubports().front().portID;
  }

  PortID findInjectPort(const std::vector<PortID>& egressPorts) {
    for (const auto& portMap :
         std::as_const(*getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        if (port->isPortUp() &&
            std::find(egressPorts.begin(), egressPorts.end(), port->getID()) ==
                egressPorts.end()) {
          return port->getID();
        }
      }
    }
    throw FbossError("No UP port found besides the mysid lag ports");
  }

  void sendPacketToProtectedSid(PortID injectPort) {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto txPacket = utility::makeIpInIpTxPacket(
        getSw(),
        getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1") /* outerSrc */,
        protectedSidPktDst() /* outerDst */,
        folly::IPAddressV6("2001:db8::1") /* innerSrc */,
        folly::IPAddressV6("2001:db8::2") /* innerDst */,
        8000 /* srcPort */,
        8001 /* dstPort */,
        0 /* outerTrafficClass */,
        0 /* innerTrafficClass */,
        64 /* hopLimit */,
        64 /* innerHopLimit */);
    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), injectPort);
  }

  // With the primary adjacency up, FRR backups are programmed but must not
  // carry traffic: the packet has to leave via the protected SID's own
  // adjacency (AGG-1) and none of the backup lags.
  void verifyForwardedViaPrimary() {
    auto primaryPort = getEgressPort(lagPortDesc(0));
    std::vector<PortID> lagPorts{primaryPort};
    for (int i = 1; i < kNumLags; ++i) {
      lagPorts.push_back(getEgressPort(lagPortDesc(i)));
    }
    auto injectPort = findInjectPort(lagPorts);

    std::map<PortID, int64_t> bytesBefore;
    for (const auto& port : lagPorts) {
      bytesBefore[port] = *getLatestPortStats(port).outBytes_();
    }

    sendPacketToProtectedSid(injectPort);

    WITH_RETRIES({
      EXPECT_EVENTUALLY_GT(
          *getLatestPortStats(primaryPort).outBytes_(),
          bytesBefore[primaryPort]);
    });

    for (int i = 1; i < kNumLags; ++i) {
      auto backupPort = getEgressPort(lagPortDesc(i));
      EXPECT_EQ(
          *getLatestPortStats(backupPort).outBytes_(), bytesBefore[backupPort])
          << "traffic egressed backup lag " << i << " on port " << backupPort;
    }
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
  }
};

TEST_F(AgentMySidAdjFrrRouteTest, addSrv6BackupProtection) {
  auto setup = [this]() { addSrv6BackupProtection(); };
  auto verify = [this]() { verifyForwardedViaPrimary(); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
