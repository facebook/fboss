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
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

/*
 * Collect stats 10K times and benchmark that.
 * Using a fixed number rather than letting framework
 * pick a N for internal iteration, since
 * - We want a large enough number to notice any memory bloat
 *   in this code path. Relying on the framework to pick a large
 *   enough iteration for us is dicey
 * - Comparing 10K iterations of 2 versions of code seems sufficient
 *   for us. Having the framework be aware that we are doing internal
 *   iteration (by letting it pick number of iterations), and calculating
 *   cost of a single iterations does not seem to have more fidelity
 */
BENCHMARK(HwStatsCollection) {
  folly::BenchmarkSuspender suspender;
  auto ensemble = createHwEnsemble({HwSwitchEnsemble::LINKSCAN});
  auto hwSwitch = ensemble->getHwSwitch();
  auto config =
      utility::onePortPerVlanConfig(hwSwitch, ensemble->masterLogicalPortIds());
  ensemble->applyInitialConfig(config);
  SwitchStats dummy;
  suspender.dismiss();
  for (auto i = 0; i < 10'000; ++i) {
    hwSwitch->updateStats(&dummy);
  }
  suspender.rehire();
}

} // namespace facebook::fboss
