// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
facebook::fboss::RoutePrefixV6 kDefaultRoute{folly::IPAddressV6(), 0};
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};
const facebook::fboss::RouterID kRid{0};
auto constexpr kEcmpWidth4 = 4;
} // namespace

namespace facebook::fboss {

class HwHashConsistencyTest : public HwLinkStateDependentTest {
 public:
  enum FlowType { TCP, UDP };

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), kRid);
    for (auto i = 0; i < kEcmpWidth4; i++) {
      ports_[i] = masterLogicalInterfacePortIds()[i];
    }

    /* experiments revealed these which L4 ports map to which switch port */
    tcpPorts_[0] = {10001, 10001};
    tcpPorts_[1] = {10005, 10005};
    tcpPorts_[2] = {10007, 10007};
    tcpPorts_[3] = {10000, 10000};
    udpPorts_ = tcpPorts_;

    auto asicType = getPlatform()->getAsic()->getAsicType();
    if (asicType == cfg::AsicType::ASIC_TYPE_EBRO) {
      tcpPortsForSai_[0] = {10002, 10002};
      tcpPortsForSai_[1] = {10004, 10004};
      tcpPortsForSai_[2] = {10000, 10000};
      tcpPortsForSai_[3] = {10001, 10001};

      udpPortsForSai_[0] = {10000, 10000};
      udpPortsForSai_[1] = {10006, 10006};
      udpPortsForSai_[2] = {10002, 10002};
      udpPortsForSai_[3] = {10003, 10003};

    } else if (
        asicType == cfg::AsicType::ASIC_TYPE_JERICHO2 ||
        asicType == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      tcpPortsForSai_[0] = {10002, 10010};
      tcpPortsForSai_[1] = {10002, 10006};
      tcpPortsForSai_[2] = {10002, 10014};
      tcpPortsForSai_[3] = {10002, 10002};
      udpPortsForSai_ = tcpPortsForSai_;

    } else {
      tcpPortsForSai_[0] = {10003, 10003};
      tcpPortsForSai_[1] = {10000, 10000};
      tcpPortsForSai_[2] = {10002, 10002};
      tcpPortsForSai_[3] = {10001, 10001};
      udpPortsForSai_ = tcpPortsForSai_;
    }
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
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
    resolve ? resolveNhops({ports_[index]}) : unresolveNhops({ports_[index]});
  }

  std::pair<uint16_t, uint16_t>
  getFlowPort(int index, bool isSai, FlowType type) const {
    switch (type) {
      case FlowType::TCP:
        return isSai ? tcpPortsForSai_[index] : tcpPorts_[index];
      case FlowType::UDP:
        return isSai ? udpPortsForSai_[index] : udpPorts_[index];
    }
  }
  std::pair<uint16_t, uint16_t> getFlowPort(int index, FlowType type) const {
    auto isSai = getHwSwitchEnsemble()->isSai();
    return getFlowPort(index, isSai, type);
  }

  void sendFlowWithPort(uint16_t l4SrcPort, uint16_t l4DstPort, FlowType type) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto dstMac = utility::getFirstInterfaceMac(getProgrammedState());

    auto tcpPkt = utility::makeTCPTxPacket(
        getHwSwitch(),
        vlanId,
        dstMac,
        dstMac,
        folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
        folly::IPAddressV6("2401:db00:11c:8200:0:0:0:104"),
        l4SrcPort,
        l4DstPort);
    auto udpPkt = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        dstMac,
        dstMac,
        folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
        folly::IPAddressV6("2401:db00:11c:8200:0:0:0:104"),
        l4SrcPort,
        l4DstPort);

    switch (type) {
      case FlowType::TCP:
        getHwSwitch()->sendPacketSwitchedSync(std::move(tcpPkt));
        break;
      case FlowType::UDP:
        getHwSwitch()->sendPacketSwitchedSync(std::move(udpPkt));
        break;
    }
  }
  void sendFlow(int index, FlowType type) {
    auto srcAndDstPort = getFlowPort(index, type);
    sendFlowWithPort(srcAndDstPort.first, srcAndDstPort.second, type);
  }
  void verifyFlowEgress(int index, FlowType flowType) {
    auto srcAndDstPort = getFlowPort(index, flowType);
    XLOG(DBG2) << " For flow ports: (" << srcAndDstPort.first << ", "
               << srcAndDstPort.second << ")"
               << " expecting increment of 1 on port at index: " << index;
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          getPortOutPkts(this->getLatestPortStats(ports_[index])), 1);
      auto pktCnt = 0;
      for (auto i = 0; i < kEcmpWidth4; i++) {
        srcAndDstPort = getFlowPort(index, flowType);
        auto pkts = getPortOutPkts(this->getLatestPortStats(ports_[i]));
        XLOG(DBG2) << " For flow ports: (" << srcAndDstPort.first << ", "
                   << srcAndDstPort.second << ")"
                   << " pkt egress increment on port at index: " << i
                   << " is: " << pkts;
        pktCnt += pkts;
      }
      EXPECT_EVENTUALLY_EQ(pktCnt, 1);
    });
  }

  void programRoute() {
    std::vector<NextHopWeight> weights(4, ECMP_WEIGHT);
    boost::container::flat_set<PortDescriptor> ports{};
    for (auto i = 0; i < kEcmpWidth4; i++) {
      ports.insert(PortDescriptor(ports_[i]));
    }
    ecmpHelper_->programRoutes(
        getRouteUpdater(), ports, {kDefaultRoute}, weights);
  }

  void setLoadBalancerSeed(cfg::LoadBalancer& cfg) {
    folly::MacAddress mac{"00:90:fb:6c:6a:94"};
    auto mac64 = mac.u64HBO();
    uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);

    auto jenkins32 = folly::hash::jenkins_rev_mix32(mac32);
    auto twang32 = folly::hash::twang_32from64(mac64);
    if (cfg.id() == cfg::LoadBalancerID::ECMP) {
      cfg.seed() = getHwSwitchEnsemble()->isSai() ? jenkins32 : twang32;
    }
    if (cfg.id() == cfg::LoadBalancerID::AGGREGATE_PORT) {
      cfg.seed() = getHwSwitchEnsemble()->isSai() ? twang32 : jenkins32;
    }
  }

  void clearPortStats() {
    auto ports = std::make_unique<std::vector<int32_t>>();
    ports->push_back(ports_[0]);
    ports->push_back(ports_[1]);
    ports->push_back(ports_[2]);
    ports->push_back(ports_[3]);
    getHwSwitch()->clearPortStats(std::move(ports));
  }

  void setupHashAndProgramRoute() {
    auto hashes = utility::getEcmpFullTrunkHalfHashConfig(getPlatform());
    for (auto& hash : hashes) {
      setLoadBalancerSeed(hash);
    }
    applyNewState(utility::addLoadBalancers(
        getPlatform(), getProgrammedState(), hashes, scopeResolver()));
    programRoute();
  }

 protected:
  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
  std::array<PortID, kEcmpWidth4> ports_{};
  std::array<std::pair<uint16_t, uint16_t>, kEcmpWidth4> tcpPorts_{};
  std::array<std::pair<uint16_t, uint16_t>, kEcmpWidth4> udpPorts_{};
  std::array<std::pair<uint16_t, uint16_t>, kEcmpWidth4> tcpPortsForSai_{};
  std::array<std::pair<uint16_t, uint16_t>, kEcmpWidth4> udpPortsForSai_{};
};

TEST_F(HwHashConsistencyTest, TcpEgressLinks) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    for (auto i = 0; i < kEcmpWidth4; i++) {
      resolveNhop(i /* ith nhop */, true /* resolve */);
    }
  };
  auto verify = [=]() {
    for (auto i = 0; i < kEcmpWidth4; i++) {
      clearPortStats();
      sendFlow(i /* flow i */, FlowType::TCP);
      verifyFlowEgress(i /* flow i */, FlowType::TCP);
    }
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
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 1);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, false /* unresolve */);
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(1 /* flow 0 */, FlowType::TCP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 0);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 2);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(1 /* flow 0 */, FlowType::TCP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwHashConsistencyTest, UdpEgressLinks) {
  auto setup = [=]() {
    setupHashAndProgramRoute();
    for (auto i = 0; i < kEcmpWidth4; i++) {
      resolveNhop(i /* ith nhop */, true /* resolve */);
    }
  };
  auto verify = [=]() {
    for (auto i = 0; i < kEcmpWidth4; i++) {
      clearPortStats();
      sendFlow(i /* ith flow  */, FlowType::UDP);
      verifyFlowEgress(i /* ith flow  */, FlowType::UDP);
    }
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
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 1);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, false /* unresolve */);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    sendFlow(1 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 0);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 2);

    clearPortStats();
    resolveNhop(0 /* nhop0 */, true /* resolve */);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    sendFlow(1 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 1);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[1])), 1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwHashConsistencyTest, VerifyMemberOrderEffect) {
  auto setup = [=]() { setupHashAndProgramRoute(); };
  auto verify = [=]() {
    for (auto i = 0; i < 4; i++) {
      resolveNhop(i /* nhop0 */, true /* resolve */);
    }
    clearPortStats();
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 2);

    clearPortStats();

    for (auto i = 0; i < 4; i++) {
      resolveNhop(i /* nhopi */, false /* resolve */);
    }
    for (auto i = 3; i >= 0; i--) {
      resolveNhop(i /* nhop0 */, true /* resolve */);
    }
    sendFlow(0 /* flow 0 */, FlowType::TCP);
    sendFlow(0 /* flow 0 */, FlowType::UDP);
    EXPECT_EQ(getPortOutPkts(this->getLatestPortStats(ports_[0])), 2);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
