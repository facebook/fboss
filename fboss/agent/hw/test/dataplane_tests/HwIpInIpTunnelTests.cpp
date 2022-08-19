// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/logging/xlog.h"

namespace facebook::fboss {

class HwIpInIpTunnelTest : public HwLinkStateDependentTest {
 protected:
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
};

TEST_F(HwIpInIpTunnelTest, TunnelDecapForwarding) {
  auto setup = [=]() {
    std::vector<cfg::IpInIpTunnel> tunnelList;
    auto cfg{this->initialConfig()};
    tunnelList.push_back(
        makeTunnelConfig("hwTestTunnel", "2401:db00:11c:8202:0:0:0:104"));
    cfg.ipInIpTunnels() = tunnelList;
    this->applyNewConfig(cfg);
  };

  auto verify = [=]() {};
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
