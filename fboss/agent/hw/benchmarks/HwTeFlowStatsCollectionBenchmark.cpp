/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

DEFINE_int32(teflow_scale_entries, 9216, "Teflow scale entries");

namespace facebook::fboss {

/*
 * Collect Teflow stats with 9K entries and benchmark that.
 */

BENCHMARK(HwTeFlowStatsCollection) {
  static std::string nextHopAddr("1::1");
  static std::string ifName("fboss2000");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_scale_entries;
  // @lint-ignore CLANGTIDY
  FLAGS_enable_exact_match = true;
  std::unique_ptr<AgentEnsemble> ensemble{};

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        CHECK_GT(ports.size(), 0);
        return utility::onePortPerInterfaceConfig(
            hwSwitch, {ports[0], ports[1]}, cfg::PortLoopbackMode::MAC);
      };

  AgentEnsemblePlatformConfigFn platformConfigFn =
      [](cfg::PlatformConfig& config) {
        if (!(config.chip()->getType() == config.chip()->bcm)) {
          return;
        }
        auto& bcm = *(config.chip()->bcm_ref());
        // enable exact match in platform config
        AgentEnsemble::enableExactMatch(bcm);
      };

  folly::BenchmarkSuspender suspender;
  ensemble = createAgentEnsemble(initialConfigFn, platformConfigFn);
  const auto& ports = ensemble->masterLogicalPortIds();
  auto hwSwitch = ensemble->getHw();
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getSw()->getState(), RouterID(0));
  // Setup EM Config
  auto state = ensemble->getSw()->getState();
  utility::setExactMatchCfg(&state, prefixLength);
  ensemble->applyNewState(state);
  // Resolve nextHops
  CHECK_GE(ports.size(), 2);
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[0])}));
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[1])}));
  // Add Entries
  auto flowEntries = utility::makeFlowEntries(
      "100", nextHopAddr, ifName, ports[0], numEntries);
  state = ensemble->getSw()->getState();
  utility::addFlowEntries(&state, flowEntries);
  ensemble->applyNewState(state, true /* rollback on fail */);
  CHECK_EQ(utility::getNumTeFlowEntries(hwSwitch), numEntries);
  // Measure stats collection time for 9K entries
  SwitchStats dummy;
  suspender.dismiss();
  hwSwitch->updateStats(&dummy);
  suspender.rehire();
}

} // namespace facebook::fboss
