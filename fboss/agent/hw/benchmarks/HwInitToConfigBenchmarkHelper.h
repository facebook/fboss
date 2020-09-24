/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Benchmark.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

using namespace facebook::fboss;

namespace facebook::fboss {

void initToConfigBenchmarkHelper(
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed) {
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  folly::BenchmarkSuspender suspender;
  auto hwSwitch = ensemble->getHwSwitch();
  auto config = utility::createUplinkDownlinkConfig(
      hwSwitch,
      ensemble->masterLogicalPortIds(),
      uplinkPortSpeed,
      downlinkPortSpeed,
      cfg::PortLoopbackMode::MAC);
  suspender.dismiss();
  ensemble->applyInitialConfig(config);
  suspender.rehire();
}

#define INIT_TO_CONFIG_BENCHMARK_HELPER(name, uplinkSpeed, downlinkSpeed) \
  BENCHMARK(name) {                                               \
    initToConfigBenchmarkHelper(uplinkSpeed, downlinkSpeed);      \
  }

} // namespace facebook::fboss
