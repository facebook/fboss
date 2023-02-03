/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwTeFlowScaleBenchmarkHelper.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

DEFINE_int32(teflow_scale_entries, 9216, "Teflow scale entries");

using namespace facebook::fboss;

namespace facebook::fboss::utility {

void teFlowAddDelEntriesBenchmarkHelper(bool measureAdd) {
  static std::string nextHopAddr("1::1");
  static std::string ifName("fboss2000");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_scale_entries;
  // @lint-ignore CLANGTIDY
  FLAGS_enable_exact_match = true;
  folly::BenchmarkSuspender suspender;

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
  auto ensemble = createAgentEnsemble(initialConfigFn, platformConfigFn);
  ensemble->startAgent();
  auto ports = ensemble->masterLogicalPortIds();
  auto hwSwitch = ensemble->getHw();
  auto state = ensemble->getSw()->getState();
  auto ecmpHelper = utility::EcmpSetupAnyNPorts6(state, RouterID(0));
  // Setup EM Config
  utility::setExactMatchCfg(&state, prefixLength);
  ensemble->applyNewState(state);
  // Resolve nextHops
  CHECK_GE(ports.size(), 2);
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[0])}));
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getSw()->getState(), {PortDescriptor(ports[1])}));
  // Add Entries
  auto flowEntries =
      makeFlowEntries("100", nextHopAddr, ifName, ports[0], numEntries);
  if (measureAdd) {
    state = ensemble->getSw()->getState();
    utility::addFlowEntries(&state, flowEntries);
    suspender.dismiss();
    state = ensemble->applyNewState(state, true /* rollback on fail */);
    suspender.rehire();
  } else {
    state = ensemble->getSw()->getState();
    utility::addFlowEntries(&state, flowEntries);
    state = ensemble->applyNewState(state, true /* rollback on fail */);
    CHECK_EQ(utility::getNumTeFlowEntries(hwSwitch), numEntries);
    utility::deleteFlowEntries(&state, flowEntries);
    suspender.dismiss();
    state = ensemble->applyNewState(state, true /* rollback on fail */);
    suspender.rehire();
  }
}

} // namespace facebook::fboss::utility
