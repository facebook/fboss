// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"

namespace {
constexpr auto kPrbsPolynomial = 9;
constexpr auto kPrbsPolynomial13 = 13;
} // unnamed namespace

namespace facebook::fboss {

class AgentPrbsTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    if (isYubaAsic(ensemble)) {
      for (auto& port : *cfg.ports()) {
        port.loopbackMode() = cfg::PortLoopbackMode::PHY;
      }
    }
    return cfg;
  }

  bool isYubaAsic(const AgentEnsemble& ensemble) const {
    if (ensemble.getNumL3Asics() >= 1) {
      auto l3Asics = ensemble.getL3Asics();
      auto asic = checkSameAndGetAsic(l3Asics);
      if (cfg::AsicType::ASIC_TYPE_YUBA == asic->getAsicType()) {
        return true;
      }
    }
    return false;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::PRBS};
  }

  std::vector<PortID> getTestPortIds() const {
    return masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT,
         cfg::PortType::FABRIC_PORT,
         cfg::PortType::MANAGEMENT_PORT});
  }

  uint16_t getPrbsPolynomial() const {
    if (getAgentEnsemble()->getNumL3Asics() >= 1) {
      auto l3Asics = getAgentEnsemble()->getL3Asics();
      auto asic = checkSameAndGetAsic(l3Asics);
      if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
        return kPrbsPolynomial13;
      }
    }
    return kPrbsPolynomial;
  }

 protected:
  void testEnablePortPrbs() {
    auto setup = [=, this]() {
      phy::PortPrbsState prbsState;
      prbsState.enabled() = true;
      prbsState.polynominal() = getPrbsPolynomial();
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        for (const auto& portId : getTestPortIds()) {
          auto port = out->getPorts()->getNodeIf(portId);
          auto newPort = port->modify(&out);
          newPort->setAsicPrbs(prbsState);
        }
        return out;
      });
    };
    auto verify = [=, this]() {
      for (const auto& portId : getTestPortIds()) {
        auto port = getProgrammedState()->getPorts()->getNodeIf(portId);
        auto asicPrbs = port->getAsicPrbs();
        EXPECT_TRUE(*asicPrbs.enabled());
        EXPECT_EQ(*asicPrbs.polynominal(), getPrbsPolynomial());
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void testDisablePortPrbs() {
    auto setup = [=, this]() {
      phy::PortPrbsState initialPrbsState;
      initialPrbsState.enabled() = true;
      initialPrbsState.polynominal() = getPrbsPolynomial();
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        for (const auto& portId : getTestPortIds()) {
          auto port = out->getPorts()->getNodeIf(portId);
          auto newPort = port->modify(&out);
          newPort->setAsicPrbs(initialPrbsState);
        }
        return out;
      });
      phy::PortPrbsState finalPrbsState;
      finalPrbsState.enabled() = false;
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        for (const auto& portId : getTestPortIds()) {
          auto port = out->getPorts()->getNodeIf(portId);
          auto newPort = port->modify(&out);
          newPort->setAsicPrbs(finalPrbsState);
        }
        return out;
      });
    };
    auto verify = [=, this]() {
      for (const auto& portId : getTestPortIds()) {
        auto port = getProgrammedState()->getPorts()->getNodeIf(portId);
        auto asicPrbs = port->getAsicPrbs();
        EXPECT_FALSE(*asicPrbs.enabled());
        EXPECT_EQ(*asicPrbs.polynominal(), 0);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentPrbsTest, enablePortPrbs) {
  testEnablePortPrbs();
}

TEST_F(AgentPrbsTest, disablePortPrbs) {
  testDisablePortPrbs();
}

} // namespace facebook::fboss
