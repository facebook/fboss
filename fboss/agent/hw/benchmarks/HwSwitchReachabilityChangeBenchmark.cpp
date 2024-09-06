/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/Benchmark.h>
#include <gtest/gtest.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_int32(max_unprocessed_switch_reachability_changes);

namespace facebook::fboss {
BENCHMARK(HwSwitchReachabilityChange) {
  folly::BenchmarkSuspender suspender;
  constexpr int kNumberOfSwitchReachabilityChangeEvents{1000};
  AgentEnsembleSwitchConfigFn initialConfig =
      [](const AgentEnsemble& ensemble) {
        // Disable DSF subscription on single-box test
        FLAGS_dsf_subscribe = false;
        // Enable fabric ports
        FLAGS_hide_fabric_ports = false;
        // Don't disable looped fabric ports
        FLAGS_disable_looped_fabric_ports = false;
        // Allow more than 1 events to be enqueued
        FLAGS_max_unprocessed_switch_reachability_changes =
            kNumberOfSwitchReachabilityChangeEvents;

        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            ensemble.masterLogicalPortIds(),
            true, /*interfaceHasSubnet*/
            true, /*setInterfaceMac*/
            utility::kBaseVlanId,
            true /*enable fabric ports*/);
        utility::populatePortExpectedNeighbors(
            ensemble.masterLogicalPortIds(), config);
        config.dsfNodes() = *utility::addRemoteDsfNodeCfg(*config.dsfNodes());
        return config;
      };
  auto ensemble =
      createAgentEnsemble(initialConfig, false /*disableLinkStateToggler*/);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::setupRemoteIntfAndSysPorts(
        in,
        ensemble->scopeResolver(),
        ensemble->getSw()->getConfig(),
        ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  });

  XLOG(DBG0) << "Wait for all fabric ports to be ACTIVE";
  // Wait for all fabric ports to be ACTIVE
  WITH_RETRIES_N_TIMED(240, std::chrono::milliseconds(1000), {
    for (const PortID& portId : ensemble->masterLogicalFabricPortIds()) {
      auto fabricPort =
          ensemble->getProgrammedState()->getPorts()->getNodeIf(portId);
      EXPECT_EVENTUALLY_TRUE(fabricPort->isActive().has_value());
      EXPECT_EVENTUALLY_TRUE(*fabricPort->isActive());
    }
  });

  XLOG(DBG0) << "Inject and wait for "
             << kNumberOfSwitchReachabilityChangeEvents
             << " switch reachability events to be processed!";
  auto reachabilityProcessedCountAtStart =
      ensemble->getSw()->stats()->getSwitchReachabilityChangeProcessed();
  int eventsInjected{0};
  suspender.dismiss();
  auto client = ensemble->getHwAgentTestClient(SwitchID(0));
  while (eventsInjected++ < kNumberOfSwitchReachabilityChangeEvents) {
    // Inject switch reachability change event
    client->sync_injectSwitchReachabilityChangeNotification();
  }
  while (ensemble->getSw()->stats()->getSwitchReachabilityChangeProcessed() !=
         (reachabilityProcessedCountAtStart +
          kNumberOfSwitchReachabilityChangeEvents)) {
    // NOOP
  }
  suspender.rehire();
}
} // namespace facebook::fboss
