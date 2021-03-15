/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/agent/rib/RoutingInformationBase.h>
#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

DECLARE_bool(enable_standalone_rib);

namespace {
std::shared_ptr<facebook::fboss::SwitchState> noopFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto hwEnsemble = static_cast<facebook::fboss::HwSwitchEnsemble*>(cookie);
  fibUpdater(hwEnsemble->getProgrammedState());
  return hwEnsemble->getProgrammedState();
}
} // namespace
namespace facebook::fboss {
BENCHMARK(RibResolutionBenchmark) {
  folly::BenchmarkSuspender suspender;
  FLAGS_enable_standalone_rib = true;
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto config = utility::onePortPerVlanConfig(
      ensemble->getHwSwitch(), ensemble->masterLogicalPortIds());
  ensemble->applyInitialConfig(config);
  utility::THAlpmRouteScaleGenerator gen(ensemble->getProgrammedState(), true);
  const auto& routeChunks = gen.getThriftRoutes();
  auto rib = ensemble->getRib();
  suspender.dismiss();
  std::for_each(
      routeChunks.begin(),
      routeChunks.end(),
      [&ensemble, rib](const auto& routeChunk) {
        rib->update(
            RouterID(0),
            ClientID::BGPD,
            AdminDistance::EBGP,
            routeChunk,
            {},
            false,
            "resolution only",
            noopFibUpdate,
            static_cast<void*>(ensemble.get()));
      });
  suspender.rehire();
}
} // namespace facebook::fboss
