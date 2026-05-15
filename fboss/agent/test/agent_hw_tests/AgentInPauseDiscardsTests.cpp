// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentInPauseDiscardsCounterTest : public AgentHwTest {
 private:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ensemble.masterLogicalPortIds());
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

 protected:
  void pumpTraffic() {
    std::vector<uint8_t> payload{0x00, 0x01, 0x00, 0x02};
    std::vector<uint8_t> padding(42, 0);
    payload.insert(payload.end(), padding.begin(), padding.end());
    auto portId = masterLogicalInterfacePortIds()[0];
    auto switchId =
        getAgentEnsemble()->scopeResolver().scope(portId).switchId();
    auto pkt = utility::makeEthTxPacket(
        getSw(),
        getVlanIDForTx(),
        getSw()->getLocalMac(switchId),
        folly::MacAddress("01:80:C2:00:00:01"),
        ETHERTYPE::ETHERTYPE_EPON,
        payload);
    getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), portId);
  }

  void commonSetup(bool enableRxPause, std::vector<PortID>& ports) {
    if (enableRxPause) {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto newState = in->clone();
        auto portMap = newState->getPorts()->modify(&newState);
        for (auto pId : ports) {
          auto port = portMap->getNodeIf(pId)->clone();
          cfg::PortPause pauseCfg;
          *pauseCfg.rx() = true;
          port->setPause(pauseCfg);
          portMap->updateNode(
              port, getAgentEnsemble()->scopeResolver().scope(port));
        }
        return newState;
      });
    }
  }

  void runTest(bool enableRxPause) {
    auto setup = [=, this]() {
      std::vector<PortID> ports = {
          PortID(this->masterLogicalInterfacePortIds()[0])};
      this->commonSetup(enableRxPause, ports);
    };
    auto verify = [=, this]() {
      auto portId = this->masterLogicalInterfacePortIds()[0];

      auto asicType = getAgentEnsemble()->getL3Asics().front()->getAsicType();
      auto asicVendor =
          getAgentEnsemble()->getL3Asics().front()->getAsicVendor();
      auto expectedPktCount = !enableRxPause &&
              (asicType == cfg::AsicType::ASIC_TYPE_EBRO ||
               asicType == cfg::AsicType::ASIC_TYPE_YUBA ||
               asicType == cfg::AsicType::ASIC_TYPE_G202X ||
               asicVendor == HwAsic::AsicVendor::ASIC_VENDOR_CHENAB)
          ? 0
          : 1;
      auto expectedDiscardsIncrement =
          isSupportedOnAllAsics(HwAsic::Feature::IN_PAUSE_INCREMENTS_DISCARDS)
          ? expectedPktCount
          : 0;
      auto portStatsBefore = getLatestPortStats(portId);
      pumpTraffic();
      WITH_RETRIES({
        auto portStatsAfter = getLatestPortStats(portId);
        EXPECT_EVENTUALLY_EQ(
            expectedDiscardsIncrement,
            *portStatsAfter.inDiscardsRaw_() -
                *portStatsBefore.inDiscardsRaw_());
        EXPECT_EVENTUALLY_EQ(
            expectedPktCount,
            *portStatsAfter.inPause_() - *portStatsBefore.inPause_());
        EXPECT_EVENTUALLY_EQ(
            0, *portStatsAfter.inDiscards_() - *portStatsBefore.inDiscards_());
      });
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentInPauseDiscardsCounterTest, rxPauseDisabled) {
  runTest(false);
}

TEST_F(AgentInPauseDiscardsCounterTest, rxPauseEnabled) {
  runTest(true);
}

class AgentInPauseFloodTest : public AgentInPauseDiscardsCounterTest {
 protected:
  void runFloodTest(bool enableRxPause) {
    auto setup = [=, this]() {
      std::vector<PortID> ports = {
          PortID(this->masterLogicalInterfacePortIds()[0]),
          PortID(this->masterLogicalInterfacePortIds()[1])};
      this->commonSetup(enableRxPause, ports);
    };
    auto verify = [=, this]() {
      auto portStatsBefore =
          getLatestPortStats(this->masterLogicalInterfacePortIds()[1]);
      pumpTraffic();
      auto portStatsAfter =
          getLatestPortStats(this->masterLogicalInterfacePortIds()[1]);
      XLOG(DBG0) << "Port " << PortID(this->masterLogicalInterfacePortIds()[1])
                 << ", outBytes, before: " << *portStatsBefore.outBytes_()
                 << ", after: " << *portStatsAfter.outBytes_();
      EXPECT_EQ(*portStatsAfter.outBytes_(), *portStatsBefore.outBytes_());
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentInPauseFloodTest, rxPauseDisabledValidateFlooding) {
  runFloodTest(false);
}

TEST_F(AgentInPauseFloodTest, rxPauseEnabledValidateFlooding) {
  runFloodTest(true);
}

} // namespace facebook::fboss
