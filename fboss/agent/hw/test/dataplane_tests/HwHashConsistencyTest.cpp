// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

namespace {
facebook::fboss::RoutePrefixV6 kDefaultRoute{folly::IPAddressV6(), 0};
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};
const facebook::fboss::RouterID kRid{0};
} // namespace

namespace facebook::fboss {

class HwHashConsistencyTest : public HwLinkStateDependentTest {
 public:
  enum FlowType { TCP, UDP };

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), kRid);
    port0_ = masterLogicalPortIds()[0];
    port1_ = masterLogicalPortIds()[1];
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
  }

  void resolveNhops(const std::vector<PortID>& ports) {
    flat_set<PortDescriptor> portDescriptors{};
    std::for_each(
        std::begin(ports), std::end(ports), [&portDescriptors](auto port) {
          portDescriptors.insert(PortDescriptor(port));
        });
    applyNewState(
        ecmpHelper_->resolveNextHops(getProgrammedState(), portDescriptors));
  }

  void unresolveNhops(const std::vector<PortID>& ports) {
    flat_set<PortDescriptor> portDescriptors{};
    std::for_each(
        std::begin(ports), std::end(ports), [&portDescriptors](auto port) {
          portDescriptors.insert(PortDescriptor(port));
        });
    applyNewState(
        ecmpHelper_->unresolveNextHops(getProgrammedState(), portDescriptors));
  }

  void resolveNhop(int index, bool resolve) {
    if (resolve) {
      index == 0 ? resolveNhops({port0_}) : resolveNhops({port1_});
    } else {
      index == 0 ? unresolveNhops({port0_}) : unresolveNhops({port1_});
    }
  }

  void sendFlow(int index, FlowType type) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto dstMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    auto port = index == 0 ? 10004 : 10005;
    auto tcpPkt = utility::makeTCPTxPacket(
        getHwSwitch(),
        vlanId,
        dstMac,
        dstMac,
        folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
        folly::IPAddressV6("2401:db00:11c:8200:0:0:0:104"),
        port,
        port);
    auto udpPkt = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        dstMac,
        dstMac,
        folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
        folly::IPAddressV6("2401:db00:11c:8200:0:0:0:104"),
        port,
        port);

    switch (type) {
      case FlowType::TCP:
        getHwSwitch()->sendPacketSwitchedSync(std::move(tcpPkt));
        break;
      case FlowType::UDP:
        getHwSwitch()->sendPacketSwitchedSync(std::move(udpPkt));
        break;
    }
  }

  void programRoute() {
    std::vector<NextHopWeight> weights(2, ECMP_WEIGHT);
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        {PortDescriptor(port0_), PortDescriptor(port1_)},
        {kDefaultRoute},
        weights);
  }

  void setLoadBalancerSeed(cfg::LoadBalancer& cfg) {
    folly::MacAddress mac{"00:90:fb:6c:6a:94"};
    auto mac64 = mac.u64HBO();
    uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);

    auto jenkins32 = folly::hash::jenkins_rev_mix32(mac32);
    auto twang32 = folly::hash::twang_32from64(mac64);
    if (cfg.id_ref() == cfg::LoadBalancerID::ECMP) {
      cfg.seed_ref() = getHwSwitchEnsemble()->isSai() ? jenkins32 : twang32;
    }
    if (cfg.id_ref() == cfg::LoadBalancerID::AGGREGATE_PORT) {
      cfg.seed_ref() = getHwSwitchEnsemble()->isSai() ? twang32 : jenkins32;
    }
  }

  void clearPortStats() {
    auto ports = std::make_unique<std::vector<int32_t>>();
    ports->push_back(port0_);
    ports->push_back(port1_);

    getHwSwitch()->clearPortStats(std::move(ports));
  }

  void setupHashAndProgramRoute() {
    auto hashes = utility::getEcmpFullTrunkHalfHashConfig(getPlatform());
    for (auto& hash : hashes) {
      setLoadBalancerSeed(hash);
    }
    applyNewState(
        utility::addLoadBalancers(getPlatform(), getProgrammedState(), hashes));
    programRoute();
  }

  void verifyPortEgress(int egressPort) {
    if (egressPort == 0) {
      EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 1);
      EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 0);
    } else {
      EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 0);
      EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 1);
    }
  }

  void verifyFlowEgress(int index, FlowType type) {
    auto isSai = getHwSwitchEnsemble()->isSai();
    if (isSai && type == FlowType::TCP) {
      /*
       * SAI uses 5-tuple where as Native SDK uses 4-tuple for hashing
       * So difference exists on which flow gets mapped to which port
       *
       * For TCP:
       * Native SDK, 10004 maps to port 0 while 10005 maps to port 1
       * SAI SDK, 10004 maps to port 1 while 10005 maps to port 0
       *
       * For UDP:
       * Native and SAI SDK, 10004 maps to port 0 and 10005 maps to port 1
       */
      index == 0 ? verifyPortEgress(1) : verifyPortEgress(0);
    } else {
      index == 0 ? verifyPortEgress(0) : verifyPortEgress(1);
    }
  }

 protected:
  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
  PortID port0_{};
  PortID port1_{};
};

TEST_F(HwHashConsistencyTest, TcpEgressLinks) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    resolveNhop(1 /* nhop1 */, true /* resolve */);
  };
  auto verify = [=]() {
    clearPortStats();
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    verifyFlowEgress(0 /* flow 0 */, FlowType::TCP);
    clearPortStats();
    sendFlow(1 /* flow 1 */, FlowType::TCP);
    verifyFlowEgress(1 /* flow 1 */, FlowType::TCP);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwHashConsistencyTest, TcpEgressLinksOnEcmpExpand) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    resolveNhop(1 /* nhop1 */, true /* resolve */);
  };
  auto verify = [=]() {
    clearPortStats();
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(1 /* flow 0 */, FlowType::TCP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 1);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, false /* unresolve */);
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(1 /* flow 0 */, FlowType::TCP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 0);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 2);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(1 /* flow 0 */, FlowType::TCP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwHashConsistencyTest, UdpEgressLinks) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    resolveNhop(1 /* nhop1 */, true /* resolve */);
  };
  auto verify = [=]() {
    clearPortStats();
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    verifyFlowEgress(0 /* flow 0 */, FlowType::UDP);
    clearPortStats();
    sendFlow(1 /* flow 1 */, FlowType::UDP);
    verifyFlowEgress(1 /* flow 0 */, FlowType::UDP);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwHashConsistencyTest, UdpEgressLinksOnEcmpExpand) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    resolveNhop(1 /* nhop0 */, true /* resolve */);
  };
  auto verify = [=]() {
    clearPortStats();
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    sendFlow(1 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 1);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, false /* unresolve */);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    sendFlow(1 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 0);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 2);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    sendFlow(1 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port0_)), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(port1_)), 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
