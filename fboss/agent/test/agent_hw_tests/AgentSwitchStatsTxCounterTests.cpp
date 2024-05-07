// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <thread>
#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {
class AgentSwitchStatsTxCounterTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::CPU_RX_TX,
        production_features::ProductionFeature::L3_FORWARDING};
  }
  std::unique_ptr<TxPacket> createL3Packet();
  void sendPacketOutOfPortAsyncAndVerifyStat(int16_t switchIndex, PortID port);
  void sendPacketAndVerifyStat(int16_t switchIndex);
  void checkTxCounters();
};

void AgentSwitchStatsTxCounterTest::checkTxCounters() {
  auto setup = []() {};
  auto verify = [=, this]() {
    getAgentEnsemble()->clearPortStats();
    const PortID port = PortID(masterLogicalPortIds()[0]);
    auto switchId = getSw()->getScopeResolver()->scope(port).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    sendPacketOutOfPortAsyncAndVerifyStat(switchIndex, port);
    sendPacketAndVerifyStat(switchIndex);
  };
  verifyAcrossWarmBoots(setup, verify);
}

std::unique_ptr<TxPacket> AgentSwitchStatsTxCounterTest::createL3Packet() {
  auto vlanId = utility::firstVlanID(getProgrammedState());
  auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
  return utility::makeUDPTxPacket(
      getSw(),
      vlanId,
      intfMac,
      intfMac,
      folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
      folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
      8000,
      8001);
}

void AgentSwitchStatsTxCounterTest::sendPacketOutOfPortAsyncAndVerifyStat(
    int16_t switchIndex,
    PortID port) {
  WITH_RETRIES({
    auto allStats = getSw()->getHwSwitchStatsExpensive();
    EXPECT_EVENTUALLY_TRUE((allStats.find(switchIndex) != allStats.end()));
  });
  auto statsBefore = getSw()->getHwSwitchStatsExpensive(switchIndex);
  auto pkt = createL3Packet();
  getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
  WITH_RETRIES({
    auto statsAfter = getSw()->getHwSwitchStatsExpensive(switchIndex);
    EXPECT_EVENTUALLY_EQ(
        *(statsAfter.fb303GlobalStats()->tx_pkt_sent()),
        *(statsBefore.fb303GlobalStats()->tx_pkt_sent()) + 1);
  });
}

void AgentSwitchStatsTxCounterTest::sendPacketAndVerifyStat(
    int16_t switchIndex) {
  WITH_RETRIES({
    auto allStats = getSw()->getHwSwitchStatsExpensive();
    EXPECT_EVENTUALLY_TRUE((allStats.find(switchIndex) != allStats.end()));
  });
  auto statsBefore = getSw()->getHwSwitchStatsExpensive(switchIndex);
  auto pkt = createL3Packet();
  getAgentEnsemble()->ensureSendPacketSwitched(std::move(pkt));
  WITH_RETRIES({
    auto statsAfter = getSw()->getHwSwitchStatsExpensive(switchIndex);
    EXPECT_EVENTUALLY_EQ(
        *(statsAfter.fb303GlobalStats()->tx_pkt_sent()),
        *(statsBefore.fb303GlobalStats()->tx_pkt_sent()) + 1);
  });
}

TEST_F(AgentSwitchStatsTxCounterTest, VerifyTxCounter) {
  checkTxCounters();
}
} // namespace facebook::fboss
