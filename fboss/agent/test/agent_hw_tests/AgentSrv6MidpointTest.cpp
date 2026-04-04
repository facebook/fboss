// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AddressUtil.h"

namespace facebook::fboss {

class AgentSrv6MidpointTest : public AgentHwTest {
 protected:
  // MySid prefix: 3001:db8:e001:: /48
  // Locator block: 3001:db8:: (32 bits), function: e001 (16 bits)
  const folly::IPAddressV6 kMySidPrefix{"3001:db8:e001::"};
  static constexpr uint8_t kMySidPrefixLen{48};

  // Input outer dst: the SID being processed at this midpoint node.
  // uSID format: [locator 3001:db8:][active uSID e001:][next uSIDs e002::]
  const folly::IPAddressV6 kPktOuterDst{"3001:db8:e001:e002::"};

  // After the active uSID e001 is processed and shifted out,
  // the rewritten outer dst forwarded to the next hop.
  const folly::IPAddressV6 kExpectedOuterDst{"3001:db8:e002::"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::SRV6_MIDPOINT};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    // Trap packets with the rewritten outer dst so the snooper can capture
    // the forwarded (uSID-shifted) packet.
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        // change mask to 128 when uSid shift is working
        std::set<folly::CIDRNetwork>{
            {folly::IPAddress("3001:db8:e002::"), 32}});
    return cfg;
  }

  utility::EcmpSetupAnyNPorts<folly::IPAddressV6> makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
  }

  void setupHelper() {
    auto ecmpHelper = makeEcmpHelper();
    this->resolveNeighbors(ecmpHelper, 1);
    addAdjacencyMySidEntry(ecmpHelper.nhop(0).ip);
  }

  void addAdjacencyMySidEntry(const folly::IPAddress& nexthopIp) {
    MySidEntry entry;
    entry.type() = MySidType::ADJACENCY_MICRO_SID;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddress(kMySidPrefix));
    prefix.prefixLength() = kMySidPrefixLen;
    entry.mySid() = prefix;

    NextHopThrift nhop;
    nhop.address() = facebook::network::toBinaryAddress(nexthopIp);
    entry.nextHops()->push_back(nhop);

    auto sw = this->getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addAdjacencyMySidEntry",
        ribMySidToSwitchStateFunc,
        sw);
  }

  void verifyMidpointForwarding(PortID egressPort) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    constexpr uint8_t kHopLimit{24};

    // Outer IPv6 dst = kPktOuterDst triggers the ADJACENCY MySid at
    // kMySidPrefix. The ASIC pops uSID e001, rewrites dst to kExpectedOuterDst,
    // and forwards out the adjacency port.
    auto txPacket = utility::makeIpInIpTxPacket(
        this->getSw(),
        this->getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1") /* outerSrc */,
        kPktOuterDst /* outerDst */,
        folly::IPAddressV6("2001:db8::1") /* innerSrc */,
        folly::IPAddressV6("2001:db8::2") /* innerDst */,
        8000 /* srcPort */,
        8001 /* dstPort */,
        0 /* outerTrafficClass */,
        0 /* innerTrafficClass */,
        kHopLimit,
        64 /* innerHopLimit */);

    utility::SwSwitchPacketSnooper snooper(
        this->getSw(), "srv6MidpointSnooper");
    this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));

    auto frameRx = snooper.waitForPacket(1);
    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(egressPort);
      EXPECT_EVENTUALLY_GT(*portStatsAfter.outBytes_(), bytesBefore);
      if (!frameRx.has_value()) {
        frameRx = snooper.waitForPacket(1);
      }
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });

    ASSERT_TRUE(frameRx.has_value());
    folly::io::Cursor cursor((*frameRx).get());
    utility::EthFrame frame(cursor);

    EXPECT_EQ(
        frame.header().etherType,
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    auto rxV6 = frame.v6PayLoad();
    ASSERT_TRUE(rxV6.has_value());
    // The active uSID (e001) has been popped; outer dst is now the next SID.
    EXPECT_EQ(rxV6->header().dstAddr, kExpectedOuterDst);
  }
};

TEST_F(AgentSrv6MidpointTest, midpointUsiShift) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = makeEcmpHelper();
    auto egressPort = ecmpHelper.nhop(0).portDesc.phyPortID();
    this->verifyMidpointForwarding(egressPort);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
