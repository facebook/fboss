/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gflags/gflags.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

cfg::SwitchConfig AgentIntegrationTest::initialConfig() const {
  cfg::SwitchConfig cfg;
  std::vector<PortID> ports;

  auto subsidiaryPortMap =
      utility::getSubsidiaryPortIDs(platform()->getPlatformPorts());
  for (auto& port : subsidiaryPortMap) {
    ports.emplace_back(port.first);
  }
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<PortMap>(), sw()->getPlatform());
  cfg = utility::onePortPerInterfaceConfig(
      platform()->getHwSwitch(), ports, cfg::PortLoopbackMode::MAC, true, true);

  cfg.switchSettings()->maxRouteCounterIDs() = 1;
  return cfg;
}

int agentIntegrationTestMain(
    int argc,
    char** argv,
    facebook::fboss::PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);

  initAgentTest(argc, argv, initPlatformFn);

  return RUN_ALL_TESTS();
}
} // namespace facebook::fboss
