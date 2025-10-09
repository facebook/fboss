/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

namespace facebook::fboss {

BENCHMARK(HwVoqScaleRouteAddBenchmark) {
  // Measure 8x512 ECMP route add
  voqRouteBenchmark(true /* add */, 8 /* ecmpGroup */, 512 /* ecmpWidth */);
}

BENCHMARK(HwVoqScale2kWideEcmpRouteAddBenchmark) {
  // Measure 16x2048 ECMP route add
  voqRouteBenchmark(true /* add */, 16 /* ecmpGroup */, 2048 /* ecmpWidth */);
}

BENCHMARK(HwVoqScale2kWideEcmpCompressionRouteAddBenchmark) {
  // Measure 50x2048 ECMP route add, which will cause EcmpResourceManager to
  // kick in and compress ecmp groups
  FLAGS_enable_ecmp_resource_manager = true;
  voqRouteBenchmark(true /* add */, 50 /* ecmpGroup */, 2048 /* ecmpWidth */);
}

BENCHMARK(HwVoqScale2kWideEcmpCompressionReconstructState) {
  // Measure 50x2048 ECMP route add, which will cause EcmpResourceManager to
  // kick in and compress ecmp groups
  FLAGS_enable_ecmp_resource_manager = true;
  folly::BenchmarkSuspender suspender;
  auto ecmpWidth = 2048;
  auto ecmpGroup = 50;
  auto ensemble = setupForVoqRouteScale(ecmpWidth);
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto remoteNhops = utility::resolveRemoteNhops(ensemble.get(), ecmpHelper);
  auto nhopSets = getVoqRouteNextHopSets(remoteNhops, ecmpGroup, ecmpWidth);
  auto prefixes = getVoqRoutePrefixes(ecmpGroup);
  auto updater = ensemble->getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(&updater, nhopSets, prefixes);
  auto l3Asics = ensemble->getL3Asics();
  auto asic = checkSameAndGetAsic(l3Asics);
  CHECK(asic->getMaxEcmpGroups().has_value());
  EcmpResourceManager resourceMgr(
      *asic->getMaxEcmpGroups(), 100 /*compressionPenaltyThresholdPct*/);
  suspender.dismiss();
  resourceMgr.reconstructFromSwitchState(ensemble->getProgrammedState());
  suspender.rehire();
}
} // namespace facebook::fboss
