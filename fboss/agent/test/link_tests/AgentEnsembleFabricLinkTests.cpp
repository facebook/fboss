// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
#include "fboss/lib/CommonUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {
const std::vector<std::string> l1LinkTestNames = {};

const std::vector<std::string> l2LinkTestNames = {"linkActiveAndLoopStatus"};
} // namespace

class AgentFabricLinkTest : public AgentEnsembleLinkTest {
 private:
  std::vector<link_test_production_features::LinkTestProductionFeature>
  getProductionFeatures() const override {
    const std::string testName =
        testing::UnitTest::GetInstance()->current_test_info()->name();

    if (std::find(l1LinkTestNames.begin(), l1LinkTestNames.end(), testName) !=
        l1LinkTestNames.end()) {
      return {link_test_production_features::LinkTestProductionFeature::
                  L1_LINK_TEST};
    } else if (
        std::find(l2LinkTestNames.begin(), l2LinkTestNames.end(), testName) !=
        l2LinkTestNames.end()) {
      return {link_test_production_features::LinkTestProductionFeature::
                  L2_LINK_TEST};
    } else {
      throw std::runtime_error(
          "Test type (L1/L2) not specified for this test case");
    }
  }
};

TEST_F(AgentFabricLinkTest, linkActiveAndLoopStatus) {
  auto connectedPairs = getConnectedPairs();
  auto checkActiveStatus = [this, &connectedPairs]() {
    WITH_RETRIES({
      for (const auto& portId : getCabledFabricPorts()) {
        auto port = getSw()->getState()->getPort(portId);
        auto peerPortId = getPeerPortID(port->getID(), connectedPairs);
        CHECK(peerPortId.has_value());
        auto peerPort = getSw()->getState()->getPort(peerPortId.value());
        auto isDrained = port->isDrained() || peerPort->isDrained();
        auto portSwitchId =
            getSw()->getScopeResolver()->scope(portId).switchId();
        auto portAsic = getSw()->getHwAsicTable()->getHwAsic(portSwitchId);
        auto expectedActiveState = PortFields::ActiveState::ACTIVE;
        if (portAsic->getSwitchType() == cfg::SwitchType::FABRIC || isDrained) {
          // Drained ports will be INACTIVE
          // On fabric switches, wrong connectivity detection will make port
          // INACTIVE
          expectedActiveState = PortFields::ActiveState::INACTIVE;
        }
        XLOG(DBG2) << " On port: " << port->getName() << " checking state: "
                   << (expectedActiveState == PortFields::ActiveState::ACTIVE
                           ? "ACTIVE"
                           : "INACTIVE");
        EXPECT_EVENTUALLY_TRUE(port->getActiveState().has_value());
        EXPECT_EVENTUALLY_EQ(
            port->getActiveState().value(), expectedActiveState);
      }
    });
  };
  auto checkLoopStatus = [this]() {
    std::vector<PortID> fabricPortsToCheck;
    for (const auto& portId : getCabledFabricPorts()) {
      auto portSwitchId = getSw()->getScopeResolver()->scope(portId).switchId();
      auto portAsic = getSw()->getHwAsicTable()->getHwAsic(portSwitchId);
      if (portAsic->getSwitchType() == cfg::SwitchType::FABRIC) {
        fabricPortsToCheck.push_back(portId);
      }
    }
    if (fabricPortsToCheck.empty()) {
      return;
    }
    WITH_RETRIES({
      auto portStats = getPortStats(fabricPortsToCheck);
      for (const auto& [portId, stats] : portStats) {
        XLOG(DBG2) << " Checking data cell filter ON for port: " << portId;
        EXPECT_EVENTUALLY_TRUE(stats.dataCellsFilterOn().has_value());
        EXPECT_EVENTUALLY_EQ(*stats.dataCellsFilterOn(), true);
      }
    });
  };
  auto setup = [this, checkActiveStatus, checkLoopStatus, &connectedPairs]() {
    checkLoopStatus();
    checkActiveStatus();
    for (const auto& portPair : connectedPairs) {
      auto port = getSw()->getState()->getPort(portPair.first);
      if (port->getPortType() == cfg::PortType::FABRIC_PORT) {
        auto cfg = getSw()->getConfig();
        for (auto& cfgPort : *cfg.ports()) {
          if (cfgPort.logicalID() == static_cast<int>(port->getID())) {
            cfgPort.drainState() = cfg::PortDrainState::DRAINED;
            break;
          }
        }
        applyNewConfig(cfg);
        break;
      }
    }
  };
  auto verify = [checkActiveStatus, checkLoopStatus]() {
    checkLoopStatus();
    checkActiveStatus();
  };
  verifyAcrossWarmBoots(setup, verify);
}
