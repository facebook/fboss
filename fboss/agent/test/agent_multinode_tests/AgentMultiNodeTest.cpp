// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"

namespace facebook::fboss {

void AgentMultiNodeTest::SetUp() {
  AgentHwTest::SetUp();
  topologyInfo_ = utility::TopologyInfo::makeTopologyInfo(getProgrammedState());

  if (!topologyInfo_->isTestDriver(*getSw())) {
    throw FbossError(
        "Agent Multi Node test binary can be run on TestDriver only");
  }
}

cfg::SwitchConfig AgentMultiNodeTest::initialConfig(
    const AgentEnsemble& ensemble) const {
  XLOG(DBG0) << "initialConfig() loaded config from file " << FLAGS_config;

  auto hwAsics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(hwAsics);

  auto it = asic->desiredLoopbackModes().find(cfg::PortType::INTERFACE_PORT);
  CHECK(it != asic->desiredLoopbackModes().end());
  auto desiredLoopbackModeForInterfacePort = it->second;

  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  auto config = *agentConfig->thrift.sw();
  for (auto& port : *config.ports()) {
    if (port.portType() == cfg::PortType::INTERFACE_PORT) {
      // TODO determine loopback mode based on ASIC type
      port.loopbackMode() = desiredLoopbackModeForInterfacePort;
    }
  }

  return config;
}

// TODO: Factor out to VOQ specific Test subclass and implement there
std::vector<ProductionFeature>
AgentMultiNodeTest::getProductionFeaturesVerified() const {
  return {ProductionFeature::VOQ};
}

void AgentMultiNodeTest::setCmdLineFlagOverrides() const {
  AgentHwTest::setCmdLineFlagOverrides();
  FLAGS_hide_fabric_ports = false;
  // Allow disabling of looped ports. This should
  // be a noop for VOQ switches
  FLAGS_disable_looped_fabric_ports = true;
  FLAGS_enable_lldp = true;
  FLAGS_tun_intf = true;
  FLAGS_disable_neighbor_updates = false;
  FLAGS_publish_stats_to_fsdb = true;
  FLAGS_publish_state_to_fsdb = true;
  FLAGS_dsf_subscribe = true;
}

void AgentMultiNodeTest::verifyCluster() const {
  switch (topologyInfo_->getTopologyType()) {
    case utility::TopologyInfo::TopologyType::DSF:
      utility::verifyDsfCluster(topologyInfo_);
      break;
  }
}

void AgentMultiNodeTest::runTestWithVerifyCluster(
    const std::function<bool(const std::unique_ptr<utility::TopologyInfo>&)>&
        testFn) const {
  // Verify cluster before running test
  verifyCluster();
  if (testing::Test::HasNonfatalFailure()) {
    // Some EXPECT_* asserts in verifyDsfClusterHelper() failed.
    FAIL()
        << "Sanity checks in DSF cluster verification failed, can't proceed with test";
  }

  // The testFn is expected to contain retries specific to the test logic,
  // thus, we can ASSERT here.
  ASSERT_TRUE(testFn(topologyInfo_));
  // Verify cluster is still healthy after test
  verifyCluster();
}

TEST_F(AgentMultiNodeTest, verifyCluster) {
  verifyCluster();
}

} // namespace facebook::fboss
