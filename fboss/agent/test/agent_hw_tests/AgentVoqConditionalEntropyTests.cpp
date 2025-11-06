// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

namespace facebook::fboss {

namespace {
constexpr auto kEcmpWidth = 4;
// Send traffic through the 5th interface port and verify load balancing
const auto kIngressPort = kEcmpWidth + 1;
constexpr auto kRehashPeriodUs = 100;
constexpr auto kReservedBytesWithRehashEnabled = 0x40;
} // namespace
class AgentVoqSwitchConditionalEntropyTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    // Enable Conditional Entropy on Interface Ports
    for (auto& port : *cfg.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT) {
        port.conditionalEntropyRehash() = true;
      }
    }
    cfg.switchSettings()->conditionalEntropyRehashPeriodUS() = kRehashPeriodUs;
    cfg.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(
            ensemble.getHwAsicTable()->getL3Asics()));
    return cfg;
  }
  std::vector<PortDescriptor> getEcmpSysPorts() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    std::vector<PortDescriptor> sysPortDescs;
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);
    sysPortDescs.insert(
        sysPortDescs.end(),
        localSysPortDescs.begin(),
        localSysPortDescs.begin() + kEcmpWidth);
    return sysPortDescs;
  }
  std::vector<SystemPortID> getEcmpSysPortIDs() {
    auto portDescs = getEcmpSysPorts();
    std::vector<SystemPortID> sysPorts;
    std::for_each(
        portDescs.begin(), portDescs.end(), [&sysPorts](auto portDesc) {
          sysPorts.push_back(portDesc.sysPortID());
        });
    return sysPorts;
  }
  void setupEcmpGroup() {
    auto sysPortDescs = getEcmpSysPorts();
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto prefix = RoutePrefixV6{folly::IPAddressV6("0::0"), 0};
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        flat_set<PortDescriptor>(
            std::make_move_iterator(sysPortDescs.begin()),
            std::make_move_iterator(sysPortDescs.end())),
        {prefix});
  }
  void clearSysPortStats(const std::vector<PortDescriptor>& sysPortDescs) {
    auto ports = std::make_unique<std::vector<int32_t>>();
    for (const auto& sysPortDecs : sysPortDescs) {
      ports->push_back(static_cast<int32_t>(sysPortDecs.sysPortID()));
    }
    getSw()->clearPortStats(ports);
  }
};

TEST_F(AgentVoqSwitchConditionalEntropyTest, verifyLoadBalancing) {
  constexpr auto kMaxDeviation = 25;

  auto setup = [this]() { setupEcmpGroup(); };

  auto verify = [this]() {
    // Send traffic through the 5th interface port and verify load balancing
    const auto kIngressPortIndex = 5;
    CHECK(masterLogicalInterfacePortIds().size() > kIngressPortIndex + 1);

    auto sysPortDescs = getEcmpSysPorts();
    std::function<std::map<SystemPortID, HwSysPortStats>(
        const std::vector<SystemPortID>&)>
        getSysPortStatsFn = [&](const std::vector<SystemPortID>& portIds) {
          getSw()->updateStats();
          return getLatestSysPortStats(portIds);
        };

    utility::pumpTrafficAndVerifyLoadBalanced(
        [&]() {
          utility::pumpRoCETraffic(
              true /* isV6 */,
              utility::getAllocatePktFn(getAgentEnsemble()),
              utility::getSendPktFunc(getAgentEnsemble()),
              utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
              std::nullopt /* vlan */,
              masterLogicalInterfacePortIds()[kIngressPortIndex],
              utility::kUdfL4DstPort,
              255 /* hopLimit */,
              std::nullopt /* srcMacAddr */,
              1000000, /* packetCount */
              utility::kUdfRoceOpcodeAck,
              kReservedBytesWithRehashEnabled, /* reserved */
              std::nullopt, /* nextHdr */
              true /* sameDstQueue */);
        },
        [&]() { clearSysPortStats(sysPortDescs); },
        [&]() {
          WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(
              utility::isLoadBalanced(
                  sysPortDescs, {}, getSysPortStatsFn, kMaxDeviation, false)));
          return true;
        });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchConditionalEntropyTest, verifyNonRoceTrafficUnbalanced) {
  auto setup = [this]() { setupEcmpGroup(); };

  auto verify = [this]() {
    CHECK(masterLogicalInterfacePortIds().size() > kIngressPort + 1);

    auto srcIp = folly::IPAddress("1001::1");
    auto dstIp = folly::IPAddress("2001::1");
    auto bytesSent = 0;
    for (auto i = 0; i < 10000; ++i) {
      auto pkt = utility::makeUDPTxPacket(
          utility::getAllocatePktFn(getAgentEnsemble()),
          std::nullopt,
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          srcIp, /* fixed */
          dstIp, /* fixed */
          42, /* arbit src port, fixed */
          24);
      bytesSent += pkt->buf()->computeChainDataLength();
      getAgentEnsemble()->sendPacketAsync(
          std::move(pkt),
          PortDescriptor(masterLogicalInterfacePortIds()[kIngressPort]),
          std::nullopt);
    }

    WITH_RETRIES({
      getSw()->updateStats();
      auto sysPortStats = getLatestSysPortStats(getEcmpSysPortIDs());
      auto [highest, lowest] = utility::getHighestAndLowestBytes(sysPortStats);
      XLOG(DBG0) << " Total bytes sent: " << bytesSent
                 << " Highest: " << highest << " Lowest: " << lowest;
      EXPECT_EVENTUALLY_GT(highest, 0.9 * bytesSent);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Conditional Entropy should be enabled if the 8th byte in the BTH header
// (reserved byte) masking with 0x40 is non-zero. This test sends packets with
// other bits in the reserved bytes set to 1 and verifies that conditional
// entropy is not enabled.
TEST_F(
    AgentVoqSwitchConditionalEntropyTest,
    verifyRoceTrafficRehashDisabledUnbalanced) {
  auto setup = [this]() { setupEcmpGroup(); };

  auto verify = [this]() {
    CHECK(masterLogicalInterfacePortIds().size() > kIngressPort + 1);

    const auto ecmpSystemPorts = getEcmpSysPortIDs();
    const auto kBitsPerByte = 8;
    const auto kPacketsToSend = 10000;
    for (int i = 0; i < kBitsPerByte; i++) {
      const uint8_t reservedBytes = 1 << i;
      if (reservedBytes == kReservedBytesWithRehashEnabled) {
        continue;
      }

      const auto beforeStats = getLatestSysPortStats(ecmpSystemPorts);
      auto packetSize = utility::pumpRoCETraffic(
          true /* isV6 */,
          utility::getAllocatePktFn(getAgentEnsemble()),
          utility::getSendPktFunc(getAgentEnsemble()),
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          std::nullopt /* vlan */,
          masterLogicalInterfacePortIds()[kIngressPort],
          utility::kUdfL4DstPort,
          255 /* hopLimit */,
          std::nullopt /* srcMacAddr */,
          kPacketsToSend, /* packetCount */
          utility::kUdfRoceOpcodeAck,
          reservedBytes,
          std::nullopt, /* nextHdr */
          true /* sameDstQueue */);
      const auto totalBytesSent = packetSize * kPacketsToSend;

      WITH_RETRIES({
        getSw()->updateStats();
        auto afterStats = getLatestSysPortStats(ecmpSystemPorts);
        auto [highest, lowest] =
            utility::getHighestAndLowestBytesIncrement(beforeStats, afterStats);
        XLOG(DBG2) << " Total bytes sent: " << totalBytesSent
                   << " Highest: " << highest << " Lowest: " << lowest;
        EXPECT_EVENTUALLY_GT(highest, 0.85 * totalBytesSent);
      });
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
