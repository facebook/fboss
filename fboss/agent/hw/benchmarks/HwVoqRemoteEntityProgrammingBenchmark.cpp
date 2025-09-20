/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwVoqRemoteEntityBenchmarkHelper.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(HwVoqRemoteNeighborAdd) {
  remoteEntityBenchmark(RemoteEntityType::REMOTE_NBR_ONLY, true /*add*/);
}

BENCHMARK(HwVoqRemoteNeighborDel) {
  remoteEntityBenchmark(RemoteEntityType::REMOTE_NBR_ONLY, false /*add*/);
}
} // namespace facebook::fboss
