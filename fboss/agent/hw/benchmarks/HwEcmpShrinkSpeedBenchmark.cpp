/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"

namespace facebook::fboss {

using utility::getEcmpSizeInHw;

BENCHMARK(HwEcmpGroupShrink) {
  folly::BenchmarkSuspender suspender;
  constexpr int kEcmpWidth = 4;
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        return utility::onePortPerInterfaceConfig(hwSwitch, ports);
      };
  auto ensemble = createAgentEnsemble(initialConfigFn);
  auto hwSwitch = ensemble->getHw();
  ensemble->setupLinkStateToggler();
  auto ports = ensemble->masterLogicalPortIds();
  ensemble->startAgent();
  auto ecmpHelper = utility::EcmpSetupAnyNPorts6(ensemble->getSw()->getState());
  ensemble->applyNewState(
      ecmpHelper.resolveNextHops(ensemble->getSw()->getState(), kEcmpWidth));
  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      kEcmpWidth);
  auto prefix = folly::CIDRNetwork(folly::IPAddress("::"), 0);
  CHECK_EQ(
      kEcmpWidth,
      getEcmpSizeInHw(hwSwitch, prefix, ecmpHelper.getRouterId(), kEcmpWidth));
  // Warm up the stats cache
  SwitchStats dummy{};
  ensemble->getHw()->updateStats(&dummy);

  // Toggle loopback mode via direct SDK calls rathe than going through
  // applyState interface. We want to start the clock ASAP post the link toggle,
  // so avoiding any extra apply state over head may give us slightly more
  // accurate reading for this micro benchmark.
  utility::setPortLoopbackMode(
      hwSwitch,
      ecmpHelper.nhop(0).portDesc.phyPortID(),
      cfg::PortLoopbackMode::NONE);
  {
    ScopedCallTimer timeIt;
    // We restart benchmarking ASAP *after* we have triggered port down
    // event above. This is analogous to starting a timer immediately
    // after a event that triggers a port down event. However, with such
    // a low interval benchmark its possible that link down handling and
    // ecmp shrink already occurs while we are restarting the timer. However,
    // if we ever introduce slowness in ecmp shrink path, we should still
    // notice it since restarting the timer/benchmark is happening at program
    // execution speed.
    // An alternative approach would be to start the timer/benchmark, before
    // triggering the link down event. Unfortunately, that's not feasible, since
    // the executing the API call to trigger link down above is relatively slow
    // - starting the timer before link down increases the benchmark time by
    // order of magnitude.
    suspender.dismiss();
    // Busy loop to see how soon after port down do we shrink ECMP group
    while (getEcmpSizeInHw(
               hwSwitch, prefix, ecmpHelper.getRouterId(), kEcmpWidth) !=
           kEcmpWidth - 1) {
      // bcm_l3_ecmp_traverse() might get stuck for not getting the mutex taken
      // by bcm_l3_ecmp_get(). Thus, sleep 1us.
      usleep(1);
    }
    suspender.rehire();
  }
}

} // namespace facebook::fboss
