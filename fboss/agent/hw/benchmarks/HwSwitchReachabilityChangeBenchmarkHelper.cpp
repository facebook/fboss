// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/Benchmark.h>
#include <gtest/gtest.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_int32(max_unprocessed_switch_reachability_changes);

using namespace facebook::fboss;

namespace facebook::fboss::utility {
void switchReachabilityChangeBenchmarkHelper(cfg::SwitchType switchType) {
  folly::BenchmarkSuspender suspender;
  constexpr int kNumberOfSwitchReachabilityChangeEvents{1000};
  // Allow more than 1 events to be enqueued
  FLAGS_max_unprocessed_switch_reachability_changes =
      kNumberOfSwitchReachabilityChangeEvents;
  AgentEnsembleSwitchConfigFn voqInitialConfig =
      [](const AgentEnsemble& ensemble) {
        // Disable DSF subscription on single-box test
        FLAGS_dsf_subscribe = false;
        // Enable fabric ports
        FLAGS_hide_fabric_ports = false;
        // Don't disable looped fabric ports
        FLAGS_disable_looped_fabric_ports = false;

        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            ensemble.masterLogicalPortIds(),
            true, /*interfaceHasSubnet*/
            true, /*setInterfaceMac*/
            utility::kBaseVlanId,
            true /*enable fabric ports*/);
        utility::populatePortExpectedNeighborsToSelf(
            ensemble.masterLogicalPortIds(), config);
        config.dsfNodes() = *utility::addRemoteIntfNodeCfg(*config.dsfNodes());
        return config;
      };
  AgentEnsembleSwitchConfigFn fabricInitialConfig =
      [](const AgentEnsemble& ensemble) {
        FLAGS_hide_fabric_ports = false;
        FLAGS_disable_looped_fabric_ports = false;
        FLAGS_detect_wrong_fabric_connections = false;
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
      };
  std::unique_ptr<AgentEnsemble> ensemble{};
  switch (switchType) {
    case cfg::SwitchType::VOQ: {
      ensemble = createAgentEnsemble(
          voqInitialConfig, false /*disableLinkStateToggler*/);
      auto updateDsfStateFn = [&ensemble](
                                  const std::shared_ptr<SwitchState>& in) {
        std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
        std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
        utility::populateRemoteIntfAndSysPorts(
            switchId2SystemPorts,
            switchId2Rifs,
            ensemble->getSw()->getConfig(),
            ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
                HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
        return DsfStateUpdaterUtil::getUpdatedState(
            in,
            ensemble->getSw()->getScopeResolver(),
            ensemble->getSw()->getRib(),
            switchId2SystemPorts,
            switchId2Rifs);
      };
      ensemble->getSw()->getRib()->updateStateInRibThread([&ensemble,
                                                           updateDsfStateFn]() {
        ensemble->getSw()->updateStateWithHwFailureProtection(
            folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
      });
    } break;
    case cfg::SwitchType::FABRIC:
      ensemble = createAgentEnsemble(
          fabricInitialConfig, false /*disableLinkStateToggler*/);
      break;
    case cfg::SwitchType::NPU:
    case cfg::SwitchType::PHY:
      throw FbossError(
          "Switch reachability benchmark unsupported on this switch type!");
  }

  XLOG(DBG0) << "Wait for all fabric ports to be ACTIVE";
  // Wait for all fabric ports to be ACTIVE
  int activeFabricPortCount;
  WITH_RETRIES_N_TIMED(240, std::chrono::milliseconds(1000), {
    activeFabricPortCount = 0;
    for (const PortID& portId : ensemble->masterLogicalFabricPortIds()) {
      auto fabricPort =
          ensemble->getProgrammedState()->getPorts()->getNodeIf(portId);
      EXPECT_EVENTUALLY_TRUE(fabricPort->isActive().has_value());
      EXPECT_EVENTUALLY_TRUE(*fabricPort->isActive());
      activeFabricPortCount++;
    }
  });

  if (activeFabricPortCount != ensemble->masterLogicalFabricPortIds().size()) {
    throw FbossError("All fabric ports are not ACTIVE yet!");
  }
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
  XLOG(DBG0)
      << "Switch reachability events processed at the start: "
      << reachabilityProcessedCountAtStart << " and at the end: "
      << ensemble->getSw()->stats()->getSwitchReachabilityChangeProcessed();
}
} // namespace facebook::fboss::utility
