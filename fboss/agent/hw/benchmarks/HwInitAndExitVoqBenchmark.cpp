/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwInitAndExitBenchmarkHelper.h"

namespace facebook::fboss {

BENCHMARK(HwInitAndExitVoqBenchmark) {
  utility::initAndExitBenchmarkHelper(
      cfg::PortSpeed::DEFAULT /* uplinkSpeed */,
      cfg::PortSpeed::DEFAULT /* downlinkSpeed */,
      cfg::SwitchType::VOQ);
}

} // namespace facebook::fboss
