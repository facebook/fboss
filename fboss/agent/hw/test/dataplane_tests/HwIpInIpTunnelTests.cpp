// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstdint>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/logging/xlog.h"

namespace facebook::fboss {

class HwIpInIpTunnelTest : public HwLinkStateDependentTest {
 protected:
  std::string kTunnelTermDstIp = "2000::1";
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }

  cfg::IpInIpTunnel makeTunnelConfig(std::string name, std::string dstIp) {
    cfg::IpInIpTunnel tunnel;
    tunnel.ipInIpTunnelId() = name;
    tunnel.underlayIntfID() =
        InterfaceID(*initialConfig().interfaces()[0].intfID());
    // in brcm-sai: masks are not supported
    tunnel.dstIp() = dstIp;
    tunnel.srcIp() = "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    tunnel.dstIpMask() = "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    tunnel.srcIpMask() = "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF";
    tunnel.ttlMode() = cfg::IpTunnelMode::PIPE;
    tunnel.dscpMode() = cfg::IpTunnelMode::PIPE;
    tunnel.ecnMode() = cfg::IpTunnelMode::PIPE;

    return tunnel;
  }
  void sendIpInIpPacket(std::string dstIpAddr) {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[1].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    const folly::IPAddressV6 outerSrcIp("1000::1");
    const folly::IPAddressV6 outerDstIp(dstIpAddr);
    const folly::IPAddressV6 innerSrcIp("4004::23");
    const folly::IPAddressV6 innerDstIp("3000::3");
    auto payload = std::vector<uint8_t>(10, 0xff);
    auto txPacket = utility::makeIpInIpTxPacket(
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
        0,
        255,
        payload);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalPortIds()[1]);
  }
};

TEST_F(HwIpInIpTunnelTest, TunnelDecapForwarding) {
  auto setup = [=]() {
    std::vector<cfg::IpInIpTunnel> tunnelList;
    auto cfg{this->initialConfig()};
    tunnelList.push_back(makeTunnelConfig("hwTestTunnel", kTunnelTermDstIp));
    cfg.ipInIpTunnels() = tunnelList;
    this->applyNewConfig(cfg);
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getPlatform()->getLocalMac());
    resolveNeigborAndProgramRoutes(
        ecmpHelper, 1); // forwarding takes the first port: 0
  };

  auto verify = [=]() {
    // src is in; dest is out
    auto beforeSrcBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto beforeDstBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();
    sendIpInIpPacket(kTunnelTermDstIp);
    auto afterSrcBytes =
        getLatestPortStats(masterLogicalPortIds()[1]).get_inBytes_();
    auto afterDstBytes =
        getLatestPortStats(masterLogicalPortIds()[0]).get_outBytes_();

    EXPECT_EQ(
        afterSrcBytes - beforeSrcBytes - IPv6Hdr::SIZE,
        afterDstBytes - beforeDstBytes);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
