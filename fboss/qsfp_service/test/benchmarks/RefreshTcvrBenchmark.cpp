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
#include <unordered_set>

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

// This function will refresh the transceivers with the specified
// media type. Besides, different hw controllers may refresh different
// number optics in parallel. In order to reduce such gaps, we refresh
// one transceiver at a time.
std::size_t refreshTcvrs(MediaInterfaceCode mediaType) {
  folly::BenchmarkSuspender suspender;
  std::size_t iters = 0;
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->init();

  for (int i = 0; i < wedgeMgr->getNumQsfpModules(); i++) {
    TransceiverID id(i);
    auto interface =
        wedgeMgr->getTransceiverInfo(id).tcvrState()->moduleMediaInterface();

    if (interface.has_value() && interface.value() == mediaType) {
      std::unordered_set<TransceiverID> tcvr{id};

      suspender.dismiss();
      wedgeMgr->TransceiverManager::refreshTransceivers(tcvr);
      suspender.rehire();
      iters++;
    }
  }

  return iters;
}

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

} // namespace facebook::fboss
