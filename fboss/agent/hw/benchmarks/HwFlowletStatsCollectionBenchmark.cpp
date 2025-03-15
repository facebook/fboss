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
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>

namespace facebook::fboss {

/*
 * Collect FlowletStats Benchmark time
 */

BENCHMARK(HwFlowletStatsCollection) {
  folly::BenchmarkSuspender suspender;
  constexpr int kEcmpWidth = 4;

  // @lint-ignore CLANGTIDY
  FLAGS_flowletSwitchingEnable = true;
  // @lint-ignore CLANGTIDY
  FLAGS_flowletStatsEnable = true;
  std::unique_ptr<AgentEnsemble> ensemble{};

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) {
        auto ports = ensemble.masterLogicalPortIds();
        auto config =
            utility::onePortPerInterfaceConfig(ensemble.getSw(), ports);
        config.udfConfig() = utility::addUdfFlowletAclConfig();
        utility::addFlowletConfigs(
            config, ensemble.masterLogicalPortIds(), ensemble.isSai());
        utility::addFlowletAcl(config);
        return config;
      };

  ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);
  // Resolve nextHops
  auto ecmpHelper = utility::EcmpSetupAnyNPorts6(ensemble->getSw()->getState());
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, kEcmpWidth);
  });
  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      kEcmpWidth);

  // Measure Flowlet stats collection time
  int iterations = 10'000;

  auto switchId = ensemble->getSw()
                      ->getScopeResolver()
                      ->scope(ensemble->masterLogicalPortIds()[0])
                      .switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);

  suspender.dismiss();
  for (auto i = 0; i < iterations; ++i) {
    client->sync_updateFlowletStats();
  }
  suspender.rehire();
}

} // namespace facebook::fboss
