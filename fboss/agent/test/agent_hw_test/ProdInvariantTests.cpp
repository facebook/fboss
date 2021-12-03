// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_test/ProdInvariantTests.h"
#include <folly/gen/Base.h>
#include <chrono>
#include <thread>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include "fboss/agent/state/Port.h"

namespace {
using facebook::fboss::PortDescriptor;
using facebook::fboss::PortID;
auto constexpr kEcmpWidth = 4;
} // namespace

namespace facebook::fboss {

void ProdInvariantTest::setupAgentTestEcmp(
    const std::vector<PortDescriptor>& ecmpPorts) {
  ASSERT_GT(ecmpPorts.size(), 0);

  boost::container::flat_set<PortDescriptor> ports;
  std::for_each(ecmpPorts.begin(), ecmpPorts.end(), [&ports](auto ecmpPort) {
    ports.insert(ecmpPort);
  });

  auto macIntf = utility::getInterfaceMac(
      sw()->getState(), (*sw()->getState()->getVlans()->begin())->getID());
  sw()->updateStateBlocking("Resolve nhops", [&](auto state) {
    utility::EcmpSetupTargetedPorts6 ecmp6(state, macIntf);
    return ecmp6.resolveNextHops(state, ports);
  });

  utility::EcmpSetupTargetedPorts6 ecmp6(sw()->getState(), macIntf);
  ecmp6.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(sw()->getRouteUpdater()),
      ports);
}

void ProdInvariantTest::SetUp() {
  AgentTest::SetUp();
  auto ecmpUplinlinkPorts =
      utility::getAllUplinkDownlinkPorts(
          platform()->getHwSwitch(), initialConfig(), kEcmpWidth, false)
          .first;
  std::vector<PortDescriptor> ecmpPorts;
  for (auto& uplinkPort : ecmpUplinlinkPorts) {
    ecmpPorts.push_back(PortDescriptor(uplinkPort));
  }
  setupAgentTestEcmp(ecmpPorts);
  XLOG(INFO) << "ProdInvariantTest setup done";
}

cfg::SwitchConfig ProdInvariantTest::initialConfig() {
  cfg::SwitchConfig cfg;
  std::vector<PortID> ports;

  auto subsidiaryPortMap =
      utility::getSubsidiaryPortIDs(platform()->getPlatformPorts());
  for (auto& port : subsidiaryPortMap) {
    ports.emplace_back(port.first);
  }
  cfg = utility::createProdRswConfig(platform()->getHwSwitch(), ports);
  return cfg;
}

void ProdInvariantTest::setupConfigFlag() {
  cfg::AgentConfig testConfig;
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<PortMap>(), platform());
  testConfig.sw_ref() = initialConfig();
  const auto& baseConfig = platform()->config();
  testConfig.platform_ref() = *baseConfig->thrift.platform_ref();
  auto newcfg = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  auto testConfigDir = platform()->getPersistentStateDir() + "/agent_test/";
  auto newCfgFile = testConfigDir + "/agent_test.conf";
  utilCreateDir(testConfigDir);
  newcfg.dumpConfig(newCfgFile);
  FLAGS_config = newCfgFile;
  // reload config so that test config is loaded
  platform()->reloadConfig();
}

void ProdInvariantTest::verifyCopp() {
  auto downlinkPort = utility::getAllUplinkDownlinkPorts(
                          sw()->getHw(), initialConfig(), kEcmpWidth, false)
                          .second[0];
  utility::verifyCoppInvariantHelper(
      sw()->getHw(),
      sw()->getPlatform()->getAsic(),
      sw()->getState(),
      downlinkPort);
}

int ProdInvariantTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

TEST_F(ProdInvariantTest, verifyCopp) {
  auto setup = [&]() {};
  auto verify = [&]() { verifyCopp(); };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
