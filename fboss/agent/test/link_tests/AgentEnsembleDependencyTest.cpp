// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"

using namespace facebook::fboss;

constexpr auto kMaxNumberOfPortsPerSwitchId = 512;

class AgentEnsembleLinkEcmpTest : public AgentEnsembleLinkTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentEnsembleLinkTest::setCmdLineFlagOverrides();
    FLAGS_ecmp_width = kMaxNumberOfPortsPerSwitchId;
  }
};

TEST_F(AgentEnsembleLinkEcmpTest, ecmpShrink) {
  auto setup = [this]() {
    const auto cabledPorts = getSingleVlanOrRoutedCabledPorts(SwitchID(0));
    programDefaultRoute(cabledPorts, getSw()->getLocalMac(scope(cabledPorts)));
  };
  auto verify = [this]() {
    auto ecmpPorts = getSingleVlanOrRoutedCabledPorts(SwitchID(0));
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    std::vector<PortID> ports;
    for (const auto& port : ecmpPorts) {
      ports.push_back(port.phyPortID());
    }
    auto verifyEcmpSize = [&](const std::shared_ptr<SwitchState>& /*state*/) {
      auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
      facebook::fboss::utility::CIDRNetwork cidr;
      cidr.IPAddress() = "::";
      cidr.mask() = 0;
      auto ecmpSizeInHw = client->sync_getHwEcmpSize(cidr, 0, ecmpPorts.size());
      auto expectedEcmpSize = ports.size();
      XLOG(DBG2) << "ecmp size in hw is " << ecmpSizeInHw
                 << " expected ecmp size " << expectedEcmpSize;
      return ecmpSizeInHw == expectedEcmpSize;
    };
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(waitForSwitchStateCondition(verifyEcmpSize));
    });

    for (const auto& port : ports) {
      setPortStatus(port, false);
    }
    EXPECT_NO_THROW(waitForLinkStatus(ports, false));
    ports.clear();
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(waitForSwitchStateCondition(verifyEcmpSize));
    });

    for (const auto& port : ecmpPorts) {
      setPortStatus(port.phyPortID(), true);
      ports.push_back(port.phyPortID());
    }
    EXPECT_NO_THROW(waitForLinkStatus(ports, true));
  };

  verifyAcrossWarmBoots(setup, verify);
}
