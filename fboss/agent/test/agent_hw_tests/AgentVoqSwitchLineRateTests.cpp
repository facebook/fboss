// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"

namespace facebook::fboss {
class AgentVoqSwitchLineRateTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentVoqSwitchTest::initialConfig(ensemble);
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  folly::MacAddress getIntfMac() const {
    return utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
  }

  void sendPacket(
      const folly::IPAddressV6& dstIp,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>()) {
    folly::IPAddressV6 kSrcIp("2402::1");
    const auto dstMac = getIntfMac();
    const auto srcMac = utility::MacAddressGenerator().get(dstMac.u64HBO() + 1);

    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        std::nullopt, // vlanID
        srcMac,
        dstMac,
        kSrcIp,
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        0x24 << 2, // dscp
        255, // hopLimit
        std::move(payload));
    // Forward the packet in the pipeline
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }

  std::vector<folly::IPAddressV6> getOneRemoteHostIpPerInterfacePort() {
    std::vector<folly::IPAddressV6> ips{};
    auto portIds = masterLogicalInterfacePortIds();
    for (int idx = 1; idx <= portIds.size(); idx++) {
      ips.emplace_back(folly::to<std::string>(2401, "::", idx));
    }
    return ips;
  }

  void setupEcmpDataplaneLoopOnAllPorts() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor(), getIntfMac());
    std::vector<PortDescriptor> portDescriptors;
    std::vector<flat_set<PortDescriptor>> portDescSets;
    for (auto& portId : masterLogicalInterfacePortIds()) {
      portDescriptors.emplace_back(portId);
      portDescSets.push_back(flat_set<PortDescriptor>{PortDescriptor(portId)});
    }
    applyNewState([&portDescriptors,
                   &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(
          in,
          flat_set<PortDescriptor>(
              std::make_move_iterator(portDescriptors.begin()),
              std::make_move_iterator(portDescriptors.end())));
    });

    std::vector<RoutePrefixV6> routePrefixes;
    for (auto prefix : getOneRemoteHostIpPerInterfacePort()) {
      routePrefixes.emplace_back(prefix, 128);
    }
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, portDescSets, routePrefixes);
  }

  auto createTrafficOnMultiplePorts(int numberOfPorts) {
    auto minPktsForLineRate = getAgentEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    auto hostIps = getOneRemoteHostIpPerInterfacePort();
    for (int idx = 0; idx < numberOfPorts; idx++) {
      for (int count = 0; count < minPktsForLineRate; count++) {
        sendPacket(hostIps[idx], std::vector<uint8_t>(1024, 0xff));
      }
    }
    // Now, make sure that we have line rate traffic on these ports!
    for (int idx = 0; idx < numberOfPorts; idx++) {
      getAgentEnsemble()->waitForLineRateOnPort(
          masterLogicalInterfacePortIds()[idx]);
    }
  }
};

TEST_F(AgentVoqSwitchLineRateTest, dramBlockedTime) {
  auto setup = [=, this]() {
    // Use just one port for the dramBlockedTime test
    // Note: If more than 3 ports are used, then traffic wouldn't
    // reach line rate which is a prerequisite for this test
    constexpr int kNumberOfPortsForDramBlock{1};
    setupEcmpDataplaneLoopOnAllPorts();
    createTrafficOnMultiplePorts(kNumberOfPortsForDramBlock);
  };
  auto verify = [=, this]() {
    // Force traffic to use DRAM to increase DRAM enqueue/dequeue!
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      int64_t dramBlockedTimeNs = 0;
      auto switchIndex =
          getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
      WITH_RETRIES({
        std::string out;
        getAgentEnsemble()->runDiagCommand(
            "m IPS_DRAM_ONLY_PROFILE DRAM_ONLY_PROFILE=-1\n", out, switchId);
        getAgentEnsemble()->runDiagCommand(
            "mod CGM_VOQ_SRAM_DRAM_MODE 0 127 VOQ_SRAM_DRAM_MODE_DATA=0x2\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "s CGM_DRAM_BOUND_STATE_TH 0\n", out, switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_WRITE_LEAKY_BUCKET_ASSERT_THRESHOLD=2\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_WRITE_LEAKY_BUCKET_DEASSERT_THRESHOLD=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_WRITE_PLUS_READ_LEAKY_BUCKET_ASSERT_THRESHOLD=2\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_WRITE_PLUS_READ_LEAKY_BUCKET_DEASSERT_THRESHOLD=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_ASSERT_THRESHOLD=2\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_DEASSERT_THRESHOLD=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_THRESHOLD_0=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_THRESHOLD_1=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_THRESHOLD_2=1\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_SIZE_0=8\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_SIZE_1=8\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand(
            "m TDU_DRAM_BLOCKED_CONFIG DRAM_BLOCKED_AVERAGE_READ_INFLIGHTS_LEAKY_BUCKET_INCREMENT_SIZE_2=8\n",
            out,
            switchId);
        getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);

        auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
        if (switchStats.fb303GlobalStats()
                ->dram_blocked_time_ns()
                .has_value()) {
          dramBlockedTimeNs =
              *switchStats.fb303GlobalStats()->dram_blocked_time_ns();
        }
        XLOG(DBG2) << "Switch ID: " << switchId
                   << ", Dram blocked time nsec: " << dramBlockedTimeNs;
        EXPECT_EVENTUALLY_GT(dramBlockedTimeNs, 0);
      });
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchLineRateTest, creditsDeleted) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    setupEcmpDataplaneLoopOnAllPorts();
    const auto kPort = masterLogicalInterfacePortIds()[0];
    auto switchId = scopeResolver().scope(kPort).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    createTrafficOnMultiplePorts(1);
    WITH_RETRIES({
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto deletedCreditBytes =
          switchStats.fb303GlobalStats()->deleted_credit_bytes().value_or(0);
      XLOG(DBG2) << "Switch ID: " << switchId
                 << ", deleted credit bytes: " << deletedCreditBytes;
      EXPECT_EVENTUALLY_GT(deletedCreditBytes, 0);
    });
    // Stop traffic loop, so that it gets restarted after warmboot
    addRemoveNeighbor(PortDescriptor(kPort), NeighborOp::DEL);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
