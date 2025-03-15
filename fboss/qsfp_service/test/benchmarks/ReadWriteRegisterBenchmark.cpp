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

BENCHMARK_MULTI(ReadRegister_CR4_100G) {
  return readOneByte(MediaInterfaceCode::CR4_100G);
}

BENCHMARK_MULTI(ReadRegister_CWDM4_100G) {
  return readOneByte(MediaInterfaceCode::CWDM4_100G);
}

BENCHMARK_MULTI(ReadRegister_FR4_200G) {
  return readOneByte(MediaInterfaceCode::FR4_200G);
}

BENCHMARK_MULTI(ReadRegister_FR4_400G) {
  return readOneByte(MediaInterfaceCode::FR4_400G);
}

BENCHMARK_MULTI(ReadRegister_LR4_400G_10KM) {
  return readOneByte(MediaInterfaceCode::LR4_400G_10KM);
}

BENCHMARK_MULTI(ReadRegister_FR4_2x400G) {
  return readOneByte(MediaInterfaceCode::FR4_2x400G);
}

BENCHMARK_MULTI(ReadRegister_DR4_2x400G) {
  return readOneByte(MediaInterfaceCode::DR4_2x400G);
}

} // namespace facebook::fboss
