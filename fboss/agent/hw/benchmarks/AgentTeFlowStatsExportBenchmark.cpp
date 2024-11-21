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
      [](const AgentEnsemble& ensemble) {
        auto ports = ensemble.masterLogicalPortIds();
        return utility::onePortPerInterfaceConfig(
            ensemble.getSw(), {ports[0], ports[1]});
        ;
      };

  AgentEnsemblePlatformConfigFn platformConfigFn =
      [](const cfg::SwitchConfig&, cfg::PlatformConfig& config) {
        if (!(config.chip()->getType() == config.chip()->bcm)) {
          return;
        }
        auto& bcm = *(config.chip()->bcm_ref());
        // enable exact match in platform config
        AgentEnsemble::enableExactMatch(bcm);
      };

  ensemble = createAgentEnsemble(
      initialConfigFn, false /*disableLinkStateToggler*/, platformConfigFn);
  const auto& ports = ensemble->masterLogicalPortIds();
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getSw()->getState(), RouterID(0));
  // Setup EM Config
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    auto state = in->clone();
    utility::setExactMatchCfg(&state, prefixLength);
    return state;
  });
  // Resolve nextHops
  CHECK_GE(ports.size(), 2);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, {PortDescriptor(ports[0])});
  });

  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, {PortDescriptor(ports[1])});
  });

  // Add Entries
  std::string dstIpStart = "100";
  auto generator = new utility::FlowEntryGenerator(
      dstIpStart, nextHopAddr, ifName, ports[0], numEntries);
  auto flowEntries = generator->generateFlowEntries();
  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in) {
        auto state = in->clone();
        utility::addFlowEntries(&state, flowEntries, ensemble->scopeResolver());
        return state;
      },
      "add-te-flow-entries",
      true /* rollback on fail */);
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
