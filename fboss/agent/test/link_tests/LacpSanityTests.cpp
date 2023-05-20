// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonFileUtils.h"

using namespace facebook::fboss;

using utility::addAggPort;
using utility::findCfgPortIf;

DECLARE_bool(enable_lacp);
DECLARE_string(config);

namespace {
constexpr int kMaxRetries{60};
constexpr int kMaxPortsPerAggPort{2};
constexpr int kBaseAggId{500};
} // unnamed namespace

class LacpTest : public LinkTest {
 private:
  void setCmdLineFlagOverrides() const override {
    FLAGS_enable_lacp = true;
    LinkTest::setCmdLineFlagOverrides();
  }

  void setupConfigFlag() override {
    XLOG(DBG2) << "setup up config flag";
    bool canWarmBoot = false;
    if (platform()->getWarmBootHelper()) {
      canWarmBoot = platform()->getWarmBootHelper()->canWarmBoot();
    } else {
      canWarmBoot =
          checkFileExists(platform()->getWarmBootDir() + "/can_warm_boot_0");
    }
    if (canWarmBoot) {
      XLOG(DBG2) << "use previous running agent config for warmboot init";
      FLAGS_config = getTestConfigPath();
      platform()->reloadConfig();
    }
  }

 public:
  void programCabledAggPorts() {
    auto config = sw()->getConfig();

    std::vector<int32_t> ports1, ports2;
    for (auto& pair : getConnectedPairs()) {
      ports1.push_back(pair.first);
      ports2.push_back(pair.second);
    }
    ports1.resize(kMaxPortsPerAggPort);
    ports2.resize(kMaxPortsPerAggPort);
    if (ports1.empty()) {
      throw FbossError("not enough ports connected for aggregate ports");
    }
    XLOG(DBG2) << "Add " << folly::join(',', ports1) << " to agg port"
               << getAggPorts()[0];
    addAggPort(getAggPorts()[0], ports1, &config, cfg::LacpPortRate::SLOW);
    XLOG(DBG2) << "Add " << folly::join(',', ports2) << " to agg port"
               << getAggPorts()[1];
    addAggPort(getAggPorts()[1], ports2, &config, cfg::LacpPortRate::SLOW);
    sw()->applyConfig("Reconfigure", config);
    dumpRunningConfig(getTestConfigPath());
  }

  std::vector<AggregatePortID> getAggPorts() const {
    return {AggregatePortID(kBaseAggId), AggregatePortID(kBaseAggId + 1)};
  }

  std::vector<LegacyAggregatePortFields::Subport> getSubPorts(
      AggregatePortID aggId) {
    const auto& aggPort =
        sw()->getState()->getMultiSwitchAggregatePorts()->getNode(aggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->sortedSubports();
  }

  // Waits for Aggregate port to be up
  void waitForAggPortStatus(AggregatePortID aggId, bool portStatus) const {
    auto aggPortUp = [&](const std::shared_ptr<SwitchState>& state) {
      const auto& aggPorts = state->getMultiSwitchAggregatePorts();
      if (aggPorts && aggPorts->getNode(aggId) &&
          aggPorts->getNode(aggId)->isUp() == portStatus) {
        return true;
      }
      return false;
    };
    EXPECT_TRUE(waitForSwitchStateCondition(aggPortUp, kMaxRetries));
  }

  // Verify that LACP converged on all member ports
  void waitForLacpState() const {
    auto lacpStateEnabled = [&](const std::shared_ptr<SwitchState>& state) {
      for (const auto& aggId : getAggPorts()) {
        const auto& aggPort =
            state->getMultiSwitchAggregatePorts()->getNode(aggId);
        if (aggPort == nullptr ||
            aggPort->forwardingSubportCount() != aggPort->subportsCount()) {
          return false;
        }
        for (const auto& memberAndState : aggPort->subportAndFwdState()) {
          // Verify that member port is enabled
          if (memberAndState.second != AggregatePort::Forwarding::ENABLED)
            return false;
          // Verify that partner has synced
          if ((aggPort->getPartnerState(memberAndState.first).state &
               LacpState::IN_SYNC) == 0) {
            return false;
          }
        }
      }
      return true;
    };
    EXPECT_TRUE(waitForSwitchStateCondition(lacpStateEnabled, kMaxRetries));
  }
};

TEST_F(LacpTest, lacpFlap) {
  auto setup = [this]() { programCabledAggPorts(); };
  auto verify = [this]() {
    const auto aggPort = getAggPorts()[0];
    waitForAggPortStatus(aggPort, true);
    waitForLacpState();

    // Local port flap
    XLOG(DBG2) << "Disable an Agg member port";
    const auto& subPorts = getSubPorts(aggPort);
    EXPECT_NE(subPorts.size(), 0);
    const auto testPort = subPorts.front().portID;
    setPortStatus(testPort, false);

    // wait for LACP protocol to react
    waitForAggPortStatus(aggPort, false);

    XLOG(DBG2) << "Enable Agg member port";
    setPortStatus(testPort, true);

    waitForAggPortStatus(aggPort, true);
    waitForLacpState();
  };

  verifyAcrossWarmBoots(setup, verify);
}
