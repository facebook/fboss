/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Benchmark.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

DECLARE_bool(setup_for_warmboot);

namespace facebook::fboss::utility {

void initandExitBenchmarkHelper(
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed);

#define INIT_AND_EXIT_BENCHMARK_HELPER(name, uplinkSpeed, downlinkSpeed) \
  BENCHMARK(name) {                                                      \
    utility::initandExitBenchmarkHelper(uplinkSpeed, downlinkSpeed);     \
  }

} // namespace facebook::fboss::utility
