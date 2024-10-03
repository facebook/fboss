// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace {
constexpr auto kPrbsPolynomial = 9;
} // unnamed namespace

namespace facebook::fboss {

class AgentPrbsTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PRBS};
  }

  std::vector<PortID> getTestPortIds() const {
    return masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT,
         cfg::PortType::FABRIC_PORT,
         cfg::PortType::MANAGEMENT_PORT});
  }

 protected:
  void testEnablePortPrbs() {
    auto setup = [=, this]() {
      phy::PortPrbsState prbsState;
      prbsState.enabled() = true;
      prbsState.polynominal() = kPrbsPolynomial;
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
        EXPECT_EQ(*asicPrbs.polynominal(), kPrbsPolynomial);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void testDisablePortPrbs() {
    auto setup = [=, this]() {
      phy::PortPrbsState initialPrbsState;
      initialPrbsState.enabled() = true;
      initialPrbsState.polynominal() = kPrbsPolynomial;
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

class AgentFabricPrbsTest : public AgentPrbsTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::FABRIC,
        production_features::ProductionFeature::PRBS};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        false /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/,
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    utility::populatePortExpectedNeighborsToSelf(
        ensemble.masterLogicalPortIds(), config);
    return config;
  }
};

TEST_F(AgentPrbsTest, enablePortPrbs) {
  testEnablePortPrbs();
}

TEST_F(AgentPrbsTest, disablePortPrbs) {
  testDisablePortPrbs();
}

TEST_F(AgentFabricPrbsTest, enablePortPrbs) {
  testEnablePortPrbs();
}

TEST_F(AgentFabricPrbsTest, disablePortPrbs) {
  testDisablePortPrbs();
}

} // namespace facebook::fboss
