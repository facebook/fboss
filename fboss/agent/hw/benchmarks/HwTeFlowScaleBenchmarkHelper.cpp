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
  static std::string dstIpStart("100");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_scale_entries;
  // @lint-ignore CLANGTIDY
  FLAGS_enable_exact_match = true;
  folly::BenchmarkSuspender suspender;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) {
        auto ports = ensemble.masterLogicalPortIds();
        CHECK_GT(ports.size(), 0);
        return utility::onePortPerInterfaceConfig(
            ensemble.getSw(), {ports[0], ports[1]});
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
  auto ports = ensemble->masterLogicalPortIds();
  // TODO(zecheng): Deprecate agent access to HwSwitch
  auto hwSwitch = ensemble->getHwSwitch();
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    // Setup EM Config
    auto state = in->clone();
    utility::setExactMatchCfg(&state, prefixLength);
    return state;
  });
  // Resolve nextHops
  CHECK_GE(ports.size(), 2);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    auto ecmpHelper = utility::EcmpSetupAnyNPorts6(in, RouterID(0));
    return ecmpHelper.resolveNextHops(
        in, {PortDescriptor(ports[0]), PortDescriptor(ports[1])});
  });
  // Add Entries
  auto flowEntries =
      makeFlowEntries(dstIpStart, nextHopAddr, ifName, ports[0], numEntries);
  if (measureAdd) {
    suspender.dismiss();
    ensemble->applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto state = in->clone();
          utility::addFlowEntries(
              &state, flowEntries, ensemble->scopeResolver());
          return state;
        },
        "add-te-flows",
        true /* rollback on fail */);
    suspender.rehire();
  } else {
    ensemble->applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto state = in->clone();
          utility::addFlowEntries(
              &state, flowEntries, ensemble->scopeResolver());
          return state;
        },
        "add-te-flows",
        true /* rollback on fail */);
    CHECK_EQ(utility::getNumTeFlowEntries(hwSwitch), numEntries);
    suspender.dismiss();
    ensemble->applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto state = in->clone();
          utility::deleteFlowEntries(&state, flowEntries);
          return state;
        },
        "del-te-flows",
        true /* rollback on fail */);
    suspender.rehire();
  }
}

} // namespace facebook::fboss::utility
