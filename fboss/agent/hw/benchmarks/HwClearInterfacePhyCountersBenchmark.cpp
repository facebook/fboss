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
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

/*
 * Collect ClearInterfacePhyCounters Benchmark Time
 */

BENCHMARK(HwClearInterfacePhyCounters) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<AgentEnsemble> ensemble{};
  int numPortsToCollectStats = 48;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) {
        return utility::onePortPerInterfaceConfig(
            ensemble.getSw(), ensemble.masterLogicalPortIds());
      };
  ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);
  int iterations = 10'000;

  std::vector<PortID> allPorts = ensemble->masterLogicalPortIds();
  const std::vector<int32_t> portIds(
      allPorts.begin(),
      allPorts.begin() +
          std::min((int)allPorts.size(), numPortsToCollectStats));

  auto switchId = ensemble->getSw()
                      ->getScopeResolver()
                      ->scope(ensemble->masterLogicalPortIds()[0])
                      .switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);
  suspender.dismiss();
  for (auto i = 0; i < iterations; ++i) {
    client->sync_clearInterfacePhyCounters(portIds);
  }
  suspender.rehire();
}

} // namespace facebook::fboss
