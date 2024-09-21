// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentPfcTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PFC};
  }

 protected:
  void sendPfcFrame(const std::vector<PortID>& portIds, uint8_t classVector) {
    for (auto portId : portIds) {
      // Construct PFC payload with fixed quanta 0x00F0.
      // See https://github.com/archjeb/pfctest for frame structure.
      std::vector<uint8_t> payload{
          0x01, 0x01, 0x00, classVector, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0,
          0x00, 0xF0, 0x00, 0xF0,        0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0,
      };
      std::vector<uint8_t> padding(26, 0);
      payload.insert(payload.end(), padding.begin(), padding.end());

      // Send it out
      auto vlanId = utility::firstVlanID(getProgrammedState());
      auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
      auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
      auto pkt = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          srcMac,
          folly::MacAddress("01:80:C2:00:00:01"), // MAC control address
          ETHERTYPE::ETHERTYPE_EPON, // Ethertype for PFC frames
          std::move(payload));
      getSw()->sendPacketOutOfPortAsync(std::move(pkt), portId);
    }
  }
};

TEST_F(AgentPfcTest, verifyPfcCounters) {
  std::vector<PortID> portIds = {
      masterLogicalInterfacePortIds()[0], masterLogicalInterfacePortIds()[1]};
  std::vector<int> losslessPgIds = {2};

  auto setup = [&]() {
    auto cfg = getAgentEnsemble()->getCurrentConfig();
    utility::setupPfcBuffers(cfg, portIds, losslessPgIds);
    applyNewConfig(cfg);

    for (auto portId : portIds) {
      auto inPfc = getLatestPortStats(portId).get_inPfc_();
      for (auto [qos, value] : inPfc) {
        EXPECT_EQ(value, 0);
      }
    }
  };

  auto verify = [&]() {
    // Collect PFC counters before test
    std::map<PortID, std::map<int, int>> inPfcBefore;
    for (auto portId : portIds) {
      auto inPfc = getLatestPortStats(portId).get_inPfc_();
      for (int pgId : losslessPgIds) {
        inPfcBefore[portId][pgId] = inPfc[pgId];
      }
    }

    sendPfcFrame(portIds, 0xFF);

    WITH_RETRIES({
      for (auto portId : portIds) {
        // We map pgIds to PFC priorities 1:1, so check for the same IDs.
        auto inPfc = getLatestPortStats(portId).get_inPfc_();
        ASSERT_EVENTUALLY_GE(inPfc.size(), losslessPgIds.size());
        for (int pgId : losslessPgIds) {
          EXPECT_EVENTUALLY_EQ(inPfc[pgId], inPfcBefore[portId][pgId] + 1);
        }
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPfcTest, verifyPfcLoopback) {
  // TODO: Investigate if this can be extended to other ASICs
  if (utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
          ->getAsicType() != cfg::AsicType::ASIC_TYPE_JERICHO3) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  std::vector<PortID> portIds = {masterLogicalInterfacePortIds()[0]};
  std::vector<int> losslessPgIds = {2};

  auto setup = [&]() {
    auto cfg = getAgentEnsemble()->getCurrentConfig();
    utility::setupPfcBuffers(cfg, portIds, losslessPgIds);
    utility::addPuntPfcPacketAcl(
        cfg, utility::getCoppMidPriQueueId(getAgentEnsemble()->getL3Asics()));
    applyNewConfig(cfg);

    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      // TODO: Investigate spurious diag command failures.
      std::string out;
      getAgentEnsemble()->runDiagCommand(
          "m DC3MAC_RSV_MASK MASK=0\n"
          "m NBU_RX_MLF_DROP_FC_PKTS RX_DROP_FC_PKTS=0\n"
          "g DC3MAC_RSV_MASK\n"
          "g NBU_RX_MLF_DROP_FC_PKTS\n"
          "",
          out,
          switchId);
      XLOG(DBG3) << "==== Diag command output ====\n"
                 << out << "\n==== End output ====";
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
    }
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    sendPfcFrame(portIds, 0xFF);

    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      ASSERT_EVENTUALLY_TRUE(frameRx.has_value());

      // Received packet should be unmodified.
      folly::io::Cursor cursor(frameRx->get());
      EthHdr ethHdr(cursor); // Consume ethernet header
      EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0101); // PFC opcode
      EXPECT_EQ(cursor.readBE<uint16_t>(), 0x00FF); // PFC class vector
      for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(cursor.readBE<uint16_t>(), 0x00F0); // PFC quanta
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
