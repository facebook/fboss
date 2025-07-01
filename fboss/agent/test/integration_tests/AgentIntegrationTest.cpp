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
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

void AgentIntegrationTest::SetUp() {
  AgentEnsembleIntegrationTestBase::SetUp();
}

cfg::SwitchConfig AgentIntegrationTest::initialConfig(
    const AgentEnsemble& ensemble) {
  cfg::SwitchConfig cfg;
  std::vector<PortID> ports;
  auto hwAsics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(hwAsics);

  auto subsidiaryPortMap =
      utility::getSubsidiaryPortIDs(ensemble.getPlatformPorts());
  for (auto& port : subsidiaryPortMap) {
    ports.emplace_back(port.first);
  }
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<MultiSwitchPortMap>(),
      ensemble.getPlatformMapping(),
      asic,
      ensemble.supportsAddRemovePort(),
      ensemble.masterLogicalPortIds());

  cfg = utility::onePortPerInterfaceConfig(
      ensemble.getSw()->getPlatformMapping(),
      asic,
      ensemble.masterLogicalPortIds(),
      ensemble.supportsAddRemovePort(),
      asic->desiredLoopbackModes() // enable all loopback modes
  );

  cfg.switchSettings()->maxRouteCounterIDs() = 1;
  return cfg;
}

int agentIntegrationTestMain(
    int argc,
    char** argv,
    facebook::fboss::PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  ::testing::InitGoogleTest(&argc, argv);

  // to ensure FLAGS_config is set, as this is used in case platform config is
  // overridden by the tests
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  initAgentEnsembleTest(argc, argv, initPlatformFn, streamType);

  return RUN_ALL_TESTS();
}
} // namespace facebook::fboss
