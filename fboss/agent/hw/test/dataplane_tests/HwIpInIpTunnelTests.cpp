// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstdint>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/logging/xlog.h"

namespace facebook::fboss {

class HwIpInIpTunnelTest : public HwLinkStateDependentTest {
 protected:
  std::string kTunnelTermDstIp = "2000::1";
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    std::vector<cfg::IpInIpTunnel> tunnelList;
    auto cfg{this->initialConfig()};
    tunnelList.push_back(makeTunnelConfig("hwTestTunnel", kTunnelTermDstIp));
    cfg.ipInIpTunnels() = tunnelList;
    this->applyNewConfig(cfg);
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getPlatform()->getLocalMac());
    resolveNeigborAndProgramRoutes(
        ecmpHelper, 1); // forwarding takes the first port: 0
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackModes());
  }

  cfg::IpInIpTunnel makeTunnelConfig(std::string name, std::string dstIp) {
    cfg::IpInIpTunnel tunnel;
    tunnel.ipInIpTunnelId() = name;
    tunnel.underlayIntfID() =
        InterfaceID(*initialConfig().interfaces()[0].intfID());
    // in brcm-sai: masks are not supported
    tunnel.dstIp() = dstIp;
    tunnel.tunnelTermType() = cfg::TunnelTerminationType::P2MP;
    tunnel.tunnelType() = cfg::TunnelType::IP_IN_IP;
    tunnel.ttlMode() = cfg::IpTunnelMode::PIPE;
    tunnel.dscpMode() = cfg::IpTunnelMode::PIPE;
    tunnel.ecnMode() = cfg::IpTunnelMode::PIPE;

    return tunnel;
  }

  std::unique_ptr<TxPacket> makePkt(
      std::string outerDstIpAddr,
      std::string innerDstIpAddr = "3000::3",
      uint8_t outerTrafficClass = 0,
      uint8_t innerTrafficClass = 0) {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[1].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    const folly::IPAddressV6 outerSrcIp("1000::1");
    const folly::IPAddressV6 outerDstIp(outerDstIpAddr);
    const folly::IPAddressV6 innerSrcIp("4004::23");
    const folly::IPAddressV6 innerDstIp(innerDstIpAddr);
    auto payload = std::vector<uint8_t>(10, 0xff);
    return utility::makeIpInIpTxPacket(
        getHwSwitch(),
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

  void sendIpInIpPacketPort(
      std::string outDstIpAddr,
      std::string innerDstIpAddr = "3000::3",
      uint8_t outerTrafficClass = 0,
      uint8_t innerTrafficClass = 0) {
    auto txPacket = makePkt(
        outDstIpAddr, innerDstIpAddr, outerTrafficClass, innerTrafficClass);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalPortIds()[1]);
  }
};

TEST_F(HwIpInIpTunnelTest, TunnelDecapForwarding) {
  auto verify = [=]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();
    sendIpInIpPacketPort(kTunnelTermDstIp);
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();

    EXPECT_EQ(
        afterInBytes - beforeInBytes - IPv6Hdr::SIZE,
        afterOutBytes - beforeOutBytes);
  };
  this->verifyAcrossWarmBoots([=] {}, verify);
}

TEST_F(HwIpInIpTunnelTest, TunnelTermEntryMiss) {
  auto verify = [=]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();
    sendIpInIpPacketPort("5000::6");
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();

    EXPECT_NE(afterInBytes, beforeInBytes);
    EXPECT_NE(afterOutBytes, beforeOutBytes);
    EXPECT_EQ(afterInBytes - beforeInBytes, afterOutBytes - beforeOutBytes);
  };
  this->verifyAcrossWarmBoots([=] {}, verify);
}

TEST_F(HwIpInIpTunnelTest, IpinIpNoTunnelConfigured) {
  auto setup = [=]() {
    auto cfg{this->initialConfig()};
    //  no tunnel configured
    this->applyNewConfig(cfg);
  };

  auto verify = [=]() {
    auto beforeInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto beforeOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();
    sendIpInIpPacketPort("5000::6");
    auto afterInBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto afterOutBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();

    EXPECT_NE(afterInBytes, beforeInBytes);
    EXPECT_NE(afterOutBytes, beforeOutBytes);
    EXPECT_EQ(afterInBytes - beforeInBytes, afterOutBytes - beforeOutBytes);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwIpInIpTunnelTest, DecapPacketParsing) {
  auto verify = [=]() {
    auto packetCapture = HwTestPacketTrapEntry(
        getHwSwitch(), std::set<PortID>({masterLogicalPortIds()[0]}));
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendIpInIpPacketPort(kTunnelTermDstIp, "dead::1", 0xFA, 0xCE);
    auto capturedPkt = snooper.waitForPacket(1);
    ASSERT_TRUE(capturedPkt.has_value());

    auto v6Pkt = capturedPkt->v6PayLoad();
    auto hdr = v6Pkt->header();
    EXPECT_EQ(hdr.dstAddr, folly::IPAddress("dead::1"));
    EXPECT_EQ(hdr.srcAddr, folly::IPAddress("4004::23"));
    // using PIPE mode: inner DSCP and ECN should not be changed
    EXPECT_EQ(hdr.trafficClass, 0xCE);

    auto udpPkt = v6Pkt->payload();
    auto payload = udpPkt->payload();
    EXPECT_EQ(payload.size(), 10);
    EXPECT_EQ(payload, std::vector<uint8_t>(10, 0xff));
    EXPECT_EQ(udpPkt->header().srcPort, 10000);
    EXPECT_EQ(udpPkt->header().dstPort, 20000);
  };

  this->verifyAcrossWarmBoots([=] {}, verify);
}

} // namespace facebook::fboss
