/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Benchmark.h>

#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

BENCHMARK_MULTI(RefreshTransceiver_CR4_100G) {
  return refreshTcvrs(MediaInterfaceCode::CR4_100G);
}

BENCHMARK_MULTI(RefreshTransceiver_CWDM4_100G) {
  return refreshTcvrs(MediaInterfaceCode::CWDM4_100G);
}

BENCHMARK_MULTI(RefreshTransceiver_FR4_200G) {
  return refreshTcvrs(MediaInterfaceCode::FR4_200G);
}

BENCHMARK_MULTI(RefreshTransceiver_FR4_400G) {
  return refreshTcvrs(MediaInterfaceCode::FR4_400G);
}

BENCHMARK_MULTI(RefreshTransceiver_LR4_400G_10KM) {
  return refreshTcvrs(MediaInterfaceCode::LR4_400G_10KM);
}

BENCHMARK_MULTI(RefreshTransceiver_DR4_2x400G) {
  return refreshTcvrs(MediaInterfaceCode::DR4_2x400G);
}

BENCHMARK_MULTI(RefreshTransceiver_FR4_2x400G) {
  return refreshTcvrs(MediaInterfaceCode::FR4_2x400G);
}

} // namespace facebook::fboss
