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
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
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

  sw()->updateStateBlocking("Resolve nhops", [&](auto state) {
    utility::EcmpSetupTargetedPorts6 ecmp6(state);
    return ecmp6.resolveNextHops(state, ports);
  });

  utility::EcmpSetupTargetedPorts6 ecmp6(sw()->getState());
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
  for (auto& uplinkPort : ecmpUplinlinkPorts) {
    ecmpPorts_.push_back(PortDescriptor(uplinkPort));
  }
  setupAgentTestEcmp(ecmpPorts_);
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

void ProdInvariantTest::sendTraffic() {
  auto mac = utility::getInterfaceMac(
      sw()->getState(), (*sw()->getState()->getVlans()->begin())->getID());
  utility::pumpTraffic(
      true,
      sw()->getHw(),
      mac,
      (*sw()->getState()->getVlans()->begin())->getID(),
      getDownlinkPort());
}

PortID ProdInvariantTest::getDownlinkPort() {
  // pick the first downlink in the list
  auto downlinkPort = utility::getAllUplinkDownlinkPorts(
                          sw()->getHw(), initialConfig(), kEcmpWidth, false)
                          .second[0];
  return downlinkPort;
}

std::map<PortID, HwPortStats> ProdInvariantTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  std::map<PortID, HwPortStats> portIdStatsMap;
  auto portNameStatsMap = sw()->getHw()->getPortStats();
  for (auto [portName, stats] : portNameStatsMap) {
    auto portId = sw()->getState()->getPorts()->getPort(portName)->getID();
    if (std::find(ports.begin(), ports.end(), (PortID)portId) == ports.end()) {
      continue;
    }
    portIdStatsMap.emplace((PortID)portId, stats);
  }
  return portIdStatsMap;
}

std::vector<PortID> ProdInvariantTest::getEcmpPortIds() {
  std::vector<PortID> ecmpPortIds{};
  for (auto portDesc : ecmpPorts_) {
    EXPECT_TRUE(portDesc.isPhysicalPort());
    auto portId = portDesc.phyPortID();
    ecmpPortIds.emplace_back(portId);
  }
  return ecmpPortIds;
}

void ProdInvariantTest::verifyLoadBalancing() {
  auto getPortStatsFn =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };
  bool loadBalanced = utility::pumpTrafficAndVerifyLoadBalanced(
      [=]() { sendTraffic(); },
      [=]() {
        auto ports = std::make_unique<std::vector<int32_t>>();
        auto ecmpPortIds = getEcmpPortIds();
        for (auto ecmpPortId : ecmpPortIds) {
          ports->push_back(static_cast<int32_t>(ecmpPortId));
        }
        sw()->getHw()->clearPortStats(ports);
      },
      [=]() {
        return utility::isLoadBalanced(
            ecmpPorts_,
            std::vector<NextHopWeight>(kEcmpWidth, 1),
            getPortStatsFn,
            25);
      });
  EXPECT_TRUE(loadBalanced);
  XLOG(INFO) << "Verify loadbalancing done";
}

void ProdInvariantTest::verifyCopp() {
  utility::verifyCoppInvariantHelper(
      sw()->getHw(),
      sw()->getPlatform()->getAsic(),
      sw()->getState(),
      getDownlinkPort());
  XLOG(INFO) << "Verify COPP done";
}

int ProdInvariantTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

TEST_F(ProdInvariantTest, verifyInvariants) {
  auto setup = [&]() {};
  auto verify = [&]() {
    verifyCopp();
    verifyLoadBalancing();
    verifyDscpToQueueMapping();
  };
  verifyAcrossWarmBoots(setup, verify);
}

void ProdInvariantTest::verifyDscpToQueueMapping() {
  if (!sw()->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto uplinkDownlinkPorts = utility::getAllUplinkDownlinkPorts(
      platform()->getHwSwitch(), initialConfig(), kEcmpWidth, false);

  // pick the first one
  auto downlinkPortId = uplinkDownlinkPorts.second[0];
  // gather all uplink + downlink ports
  std::vector<PortID> portIds = uplinkDownlinkPorts.first;
  for (auto it = uplinkDownlinkPorts.second.begin();
       it != uplinkDownlinkPorts.second.end();
       ++it) {
    portIds.push_back(*it);
  }

  auto getPortStatsFn = [&]() -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };

  auto q2dscpMap = utility::getOlympicQosMaps(initialConfig());
  EXPECT_TRUE(utility::verifyQueueMappingsInvariantHelper(
      q2dscpMap,
      sw()->getHw(),
      sw()->getState(),
      getPortStatsFn,
      getEcmpPortIds(),
      downlinkPortId));

  XLOG(INFO) << "Verify DSCP to Queue mapping done";
}

} // namespace facebook::fboss
