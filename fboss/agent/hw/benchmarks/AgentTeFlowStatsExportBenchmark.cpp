/*
 *  Copyright (c) 20023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

DEFINE_int32(teflow_em_entries, 10000, "TEFlow EM entries");
DEFINE_int32(fsdb_connect_timeout, 60, "FSDB connect timeout in seconds");

namespace facebook::fboss {

BENCHMARK(AgentTeFlowStatsPublishToFsdb) {
  folly::BenchmarkSuspender suspender;

  static std::string nextHopAddr("1::1");
  static std::string ifName("fboss2000");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_em_entries;
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

  ensemble = createAgentEnsemble(initialConfigFn, platformConfigFn);
  const auto& ports = ensemble->masterLogicalPortIds();
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
  std::string dstIpStart = "100";
  auto generator = new utility::FlowEntryGenerator(
      dstIpStart, nextHopAddr, ifName, ports[0], numEntries);
  auto flowEntries = generator->generateFlowEntries();
  state = ensemble->getSw()->getState();
  utility::addFlowEntries(&state, flowEntries, ensemble->scopeResolver());
  ensemble->applyNewState(state, true /* rollback on fail */);
  // verify TeFlowStats size
  auto teFlowStats = ensemble->getSw()->getTeFlowStats();
  auto hwTeFlowStats = teFlowStats.hwTeFlowStats();
  CHECK_EQ(hwTeFlowStats->size(), numEntries);

  // wait for FsdbSyncer to be ready to publish stats to FSDB
  auto waitForFsdbConnection = [&ensemble](int timeout) -> bool {
    for (int i = 0; i < timeout; i++) {
      if (ensemble->getSw()->fsdbStatPublishReady()) {
        return true;
      }
      // @lint-ignore CLANGTIDY
      sleep(1);
    }
    return false;
  };
  CHECK_EQ(waitForFsdbConnection(FLAGS_fsdb_connect_timeout), true);

  // benchmark time for publishing stat to FSDB
  suspender.dismiss();
  ensemble->getSw()->publishStatsToFsdb();
  suspender.rehire();
}

} // namespace facebook::fboss
