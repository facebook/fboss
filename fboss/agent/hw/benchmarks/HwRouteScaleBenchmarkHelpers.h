/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"

#include <folly/Benchmark.h>
#include <iostream>
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss {

/*
 * Helper function to benchmark speed of route insertion, deletion
 * in HW. This function inits the ASIC, generate switch states for
 * a given route distribution and then measures the time it takes
 * to add (or delete post addition) these routes
 */
template <typename RouteScaleGeneratorT>
void routeAddDelBenchmarker(bool measureAdd) {
  folly::BenchmarkSuspender suspender;
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        return utility::onePortPerInterfaceConfig(hwSwitch, ports);
      };
  auto ensemble = createAgentEnsemble(initialConfigFn);
  auto* sw = ensemble->getSw();

  auto routeGenerator = RouteScaleGeneratorT(sw->getState());
  if (!routeGenerator.isSupported(ensemble->getPlatform()->getType())) {
    // skip if this is not supported for a platform
    return;
  }
  ensemble->applyNewState(routeGenerator.resolveNextHops(sw->getState()));
  const RouterID kRid(0);
  auto routeChunks = routeGenerator.getThriftRoutes();
  auto allThriftRoutes = routeGenerator.allThriftRoutes();
  // Get routes with one smaller ecmp width to capture a peering
  // flap and following route updates
  auto allThriftRoutesNarrowerEcmp = RouteScaleGeneratorT(
                                         sw->getState(),
                                         allThriftRoutes.size(),
                                         routeGenerator.ecmpWidth() - 1)
                                         .allThriftRoutes();

  std::atomic<bool> done{false};

  auto doLookups = [&ensemble, kRid, &done]() {
    auto programmedState = ensemble->getSw()->getState();
    std::vector<folly::IPAddressV6> addrsToLookup;
    utility::IPAddressGenerator<folly::IPAddressV6> ipAddrGen;
    for (auto i = 0; i < 1000; ++i) {
      addrsToLookup.emplace_back(ipAddrGen.getNext());
    }
    double worstCaseLookupMsecs = 0;
    double worstCaseBulkLookupMsecs = 0;
    auto recordMaxLookupTime = [](const StopWatch& timer,
                                  double& curMaxTimeMsecs) {
      auto msecsElapsed = timer.msecsElapsed().count();
      curMaxTimeMsecs =
          msecsElapsed > curMaxTimeMsecs ? msecsElapsed : curMaxTimeMsecs;
    };
    while (!done) {
      {
        StopWatch bulkLookupTimer(std::nullopt, FLAGS_json);
        std::for_each(
            addrsToLookup.begin(),
            addrsToLookup.end(),
            [&ensemble,
             &programmedState,
             kRid,
             &worstCaseLookupMsecs,
             &recordMaxLookupTime](const auto& addr) {
              StopWatch lookupTimer(std::nullopt, FLAGS_json);
              findLongestMatchRoute(
                  ensemble->getSw()->getRib(), kRid, addr, programmedState);
              recordMaxLookupTime(lookupTimer, worstCaseLookupMsecs);
            });
        recordMaxLookupTime(bulkLookupTimer, worstCaseBulkLookupMsecs);
      }
      // Give some breathing room so the thread doesn't  eat all its quantum
      // of ticks and gets scheduled out in middle of loookups
      usleep(1000);
    }
    if (FLAGS_json) {
      folly::dynamic time = folly::dynamic::object;
      time["worst_case_lookup_msescs"] = worstCaseLookupMsecs;
      time["worst_case_bulk_lookup_msescs"] = worstCaseBulkLookupMsecs;
      std::cout << toPrettyJson(time) << std::endl;
    } else {
      XLOG(DBG2) << "worst_case_lookup_msescs : " << worstCaseLookupMsecs
                 << " worst_case_bulk_lookup_msescs : "
                 << worstCaseBulkLookupMsecs;
    }
  };
  // Start parallel lookup thread
  std::thread lookupThread([&doLookups]() { doLookups(); });
  auto updater = ensemble->getSw()->getRouteUpdater();
  if (measureAdd) {
    {
      // Route add benchmark
      ScopedCallTimer timeIt;
      // Activate benchmarker before applying switch states
      // for adding routes to h/w
      suspender.dismiss();
      // Program 1 chunk to seed ~4k routes
      // program remaining chunks
      ensemble->programRoutes(RouterID(0), ClientID::BGPD, routeChunks);
      // We are about to blow away all routes, before that
      // deactivate benchmark measurement.
      suspender.rehire();
    }
    // Do a sync fib and have it compete with route lookups
    auto syncFib =
        [&updater,
         kRid](const utility::RouteDistributionGenerator::ThriftRouteChunk&
                   routes) {
          std::for_each(
              routes.begin(),
              routes.end(),
              [&updater, kRid](const auto& route) {
                updater.addRoute(kRid, ClientID::BGPD, route);
              });
          updater.program(
              {{{kRid, ClientID::BGPD}},
               RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY});
        };
    // Sync fib with same set of routes
    syncFib(allThriftRoutes);
    // Sync fib with same set of routes, but ecmp nhops changed
    syncFib(allThriftRoutesNarrowerEcmp);
    // Sync fib with no routes - del all
    syncFib({});
    // Sync fib with all routes
    syncFib(allThriftRoutes);
  } else {
    ensemble->programRoutes(kRid, ClientID::BGPD, routeChunks);
    ScopedCallTimer timeIt;
    // We are about to blow away all routes, before that
    // activate benchmark measurement.
    suspender.dismiss();
    ensemble->unprogramRoutes(kRid, ClientID::BGPD, routeChunks);
    suspender.rehire();
  }
  done = true;
  lookupThread.join();
}

#define ROUTE_ADD_BENCHMARK(name, RouteScaleGeneratorT) \
  BENCHMARK(name) {                                     \
    routeAddDelBenchmarker<RouteScaleGeneratorT>(true); \
  }

#define ROUTE_DEL_BENCHMARK(name, RouteScaleGeneratorT)  \
  BENCHMARK(name) {                                      \
    routeAddDelBenchmarker<RouteScaleGeneratorT>(false); \
  }

} // namespace facebook::fboss
