// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Benchmark.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utility {

void switchReachabilityChangeBenchmarkHelper(cfg::SwitchType switchType);

} // namespace facebook::fboss::utility
