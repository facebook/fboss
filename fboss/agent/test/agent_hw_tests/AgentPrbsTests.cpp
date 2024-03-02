// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"
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
};

TEST_F(AgentPrbsTest, enablePortPrbs) {
  auto setup = [=, this]() {
    EXPECT_GT(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(), 0);
    auto interfacePortId =
        PortID(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    phy::PortPrbsState prbsState;
    prbsState.enabled() = true;
    prbsState.polynominal() = kPrbsPolynomial;
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(interfacePortId);
      auto newPort = port->modify(&out);
      newPort->setAsicPrbs(prbsState);
      return out;
    });
  };
  auto verify = [=, this]() {
    EXPECT_GT(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(), 0);
    auto interfacePortId =
        PortID(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    auto port = getProgrammedState()->getPorts()->getNodeIf(interfacePortId);
    auto asicPrbs = port->getAsicPrbs();
    EXPECT_TRUE(*asicPrbs.enabled());
    EXPECT_EQ(*asicPrbs.polynominal(), kPrbsPolynomial);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPrbsTest, disablePortPrbs) {
  auto setup = [=, this]() {
    EXPECT_GT(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(), 0);
    auto interfacePortId =
        PortID(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    phy::PortPrbsState initialPrbsState;
    initialPrbsState.enabled() = true;
    initialPrbsState.polynominal() = kPrbsPolynomial;
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(interfacePortId);
      auto newPort = port->modify(&out);
      newPort->setAsicPrbs(initialPrbsState);
      return out;
    });
    phy::PortPrbsState finalPrbsState;
    finalPrbsState.enabled() = false;
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(interfacePortId);
      auto newPort = port->modify(&out);
      newPort->setAsicPrbs(finalPrbsState);
      return out;
    });
  };
  auto verify = [=, this]() {
    EXPECT_GT(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(), 0);
    auto interfacePortId =
        PortID(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    auto port = getProgrammedState()->getPorts()->getNodeIf(interfacePortId);
    auto asicPrbs = port->getAsicPrbs();
    EXPECT_FALSE(*asicPrbs.enabled());
    EXPECT_EQ(*asicPrbs.polynominal(), 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
