/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

namespace facebook::fboss {

class AgentIpInIpTunnelTest : public AgentHwTest {
 protected:
  std::string kTunnelTermDstIp = "2000::1";
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::IP_IN_IP_DECAP};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = checkSameAndGetAsic(l3Asics);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    addTunnelConfig(cfg, asic);
    auto port = ensemble.masterLogicalInterfacePortIds()[0];
    utility::addTrapPacketAcl(asic, &cfg, port);
    return cfg;
  }

  void setupHelper() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    resolveNeighborAndProgramRoutes(
        ecmpHelper, 1); // forwarding takes the first port: 0
  }

  void sendIpInIpPacketPort(
      std::string outDstIpAddr,
      std::string innerDstIpAddr = "3000::3",
      uint8_t outerTrafficClass = 0,
      uint8_t innerTrafficClass = 0) {
    auto txPacket = makePkt(
        outDstIpAddr, innerDstIpAddr, outerTrafficClass, innerTrafficClass);
    getAgentEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalPortIds()[1]);
  }

 private:
  void addTunnelConfig(cfg::SwitchConfig& cfg, const HwAsic* asic) const {
    std::vector<cfg::IpInIpTunnel> tunnelList;
    tunnelList.push_back(makeTunnelConfig(
        "hwTestTunnel",
        kTunnelTermDstIp,
        (InterfaceID)cfg.interfaces()[0].intfID().value(),
        asic));
    cfg.ipInIpTunnels() = tunnelList;
  }

  cfg::IpInIpTunnel makeTunnelConfig(
      std::string name,
      std::string dstIp,
      InterfaceID interfaceId,
      const HwAsic* asic) const {
    cfg::IpInIpTunnel tunnel;
    tunnel.ipInIpTunnelId() = name;
    tunnel.underlayIntfID() = interfaceId;
    // in brcm-sai: masks are not supported
    tunnel.dstIp() = dstIp;
    tunnel.tunnelTermType() = cfg::TunnelTerminationType::P2MP;
    tunnel.tunnelType() = cfg::TunnelType::IP_IN_IP;
    tunnel.ttlMode() = cfg::IpTunnelMode::PIPE;
    tunnel.dscpMode() = asic->getTunnelDscpMode();
    tunnel.ecnMode() = cfg::IpTunnelMode::PIPE;

    return tunnel;
  }

  std::unique_ptr<TxPacket> makePkt(
      std::string outerDstIpAddr,
      std::string innerDstIpAddr = "3000::3",
      uint8_t outerTrafficClass = 0,
      uint8_t innerTrafficClass = 0) {
    auto vlanId =
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    const folly::IPAddressV6 outerSrcIp("1000::1");
    const folly::IPAddressV6 outerDstIp(outerDstIpAddr);
    const folly::IPAddressV6 innerSrcIp("4004::23");
    const folly::IPAddressV6 innerDstIp(innerDstIpAddr);
    auto payload = std::vector<uint8_t>(10, 0xff);
    return utility::makeIpInIpTxPacket(
        getSw(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        outerSrcIp,
        outerDstIp,
        innerSrcIp,
        innerDstIp,
        10000, // udp src port
        20000, // udp dst port
        outerTrafficClass,
        innerTrafficClass,
        255,
        payload);
  }
};

TEST_F(AgentIpInIpTunnelTest, TunnelDecapForwarding) {
  auto setup = [=, this]() { setupHelper(); };
  auto verify = [=, this]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();
    sendIpInIpPacketPort(kTunnelTermDstIp);
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();

    EXPECT_EQ(
        afterInBytes - beforeInBytes - IPv6Hdr::SIZE,
        afterOutBytes - beforeOutBytes);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentIpInIpTunnelTest, TunnelTermEntryMiss) {
  auto setup = [=, this]() { setupHelper(); };
  auto verify = [=, this]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();
    sendIpInIpPacketPort("5000::6");
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();

    EXPECT_NE(afterInBytes, beforeInBytes);
    EXPECT_NE(afterOutBytes, beforeOutBytes);
    EXPECT_EQ(afterInBytes - beforeInBytes, afterOutBytes - beforeOutBytes);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentIpInIpTunnelTest, IpinIpNoTunnelConfigured) {
  auto setup = [=, this]() {
    auto ensemble = getAgentEnsemble();
    auto l3Asics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
    auto asic = checkSameAndGetAsic(l3Asics);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble->getSw()->getPlatformMapping(),
        asic,
        ensemble->masterLogicalPortIds()[0],
        ensemble->masterLogicalPortIds()[1],
        ensemble->getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    //  no tunnel configured
    applyNewConfig(cfg);
    setupHelper();
  };

  auto verify = [=, this]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();
    sendIpInIpPacketPort("5000::6");
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).inBytes_().value();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).outBytes_().value();

    EXPECT_NE(afterInBytes, beforeInBytes);
    EXPECT_NE(afterOutBytes, beforeOutBytes);
    EXPECT_EQ(afterInBytes - beforeInBytes, afterOutBytes - beforeOutBytes);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentIpInIpTunnelTest, DecapPacketParsing) {
  auto setup = [=, this]() { setupHelper(); };
  auto verify = [=, this]() {
    auto ensemble = getAgentEnsemble();
    auto l3Asics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
    auto asic = checkSameAndGetAsic(l3Asics);

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    sendIpInIpPacketPort(kTunnelTermDstIp, "dead::1", 0xFA, 0xCE);
    auto capturedPktBuf = snooper.waitForPacket(1);
    ASSERT_TRUE(capturedPktBuf.has_value());

    folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
    auto capturedPkt = utility::EthFrame(capturedPktCursor);
    auto v6Pkt = capturedPkt.v6PayLoad();
    auto hdr = v6Pkt->header();
    EXPECT_EQ(hdr.dstAddr, folly::IPAddress("dead::1"));
    EXPECT_EQ(hdr.srcAddr, folly::IPAddress("4004::23"));

    if (cfg::IpTunnelMode::PIPE == asic->getTunnelDscpMode()) {
      // using PIPE mode: inner DSCP and ECN should not be changed
      EXPECT_EQ(hdr.trafficClass, 0xCE);
    } else {
      // using UNIFORM mode: Where the DSCP field is preserved end-to-end by
      // copying into the outer header on encapsulation and copying from the
      // outer header on decapsulation.
      EXPECT_EQ(hdr.trafficClass, 0xF8);
    }

    auto udpPkt = v6Pkt->udpPayload();
    auto payload = udpPkt->payload();
    EXPECT_EQ(payload.size(), 10);
    EXPECT_EQ(payload, std::vector<uint8_t>(10, 0xff));
    EXPECT_EQ(udpPkt->header().srcPort, 10000);
    EXPECT_EQ(udpPkt->header().dstPort, 20000);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
