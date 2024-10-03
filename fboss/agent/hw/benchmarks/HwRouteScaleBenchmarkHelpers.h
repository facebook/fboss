/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

#include <folly/Benchmark.h>
#include <iostream>
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include "fboss/agent/test/AgentEnsemble.h"

namespace {
const auto kEcmpWidth = 512;
const auto kEcmpGroup = 8;
}; // namespace

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
      [](const AgentEnsemble& ensemble) {
        return utility::onePortPerInterfaceConfig(
            ensemble.getSw(), ensemble.masterLogicalPortIds());
      };
  auto ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);
  auto* sw = ensemble->getSw();

  auto routeGenerator = RouteScaleGeneratorT(sw->getState());
  auto swSwitch = ensemble->agentInitializer()->sw();
  auto platformType = swSwitch->getPlatformType();
  if (!routeGenerator.isSupported(platformType)) {
    // skip if this is not supported for a platform
    return;
  }
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return routeGenerator.resolveNextHops(in);
  });
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

inline void voqRouteBenchmark(bool add) {
  folly::BenchmarkSuspender suspender;

  AgentEnsembleSwitchConfigFn voqInitialConfig =
      [](const AgentEnsemble& ensemble) {
        FLAGS_hide_fabric_ports = false;
        FLAGS_dsf_subscribe = false;
        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            ensemble.masterLogicalPortIds(),
            true, /*interfaceHasSubnet*/
            true, /*setInterfaceMac*/
            utility::kBaseVlanId,
            true /*enable fabric ports*/);
        utility::populatePortExpectedNeighborsToSelf(
            ensemble.masterLogicalPortIds(), config);
        config.dsfNodes() = *utility::addRemoteIntfNodeCfg(*config.dsfNodes());
        return config;
      };
  auto ensemble =
      createAgentEnsemble(voqInitialConfig, false /*disableLinkStateToggler*/);
  ScopedCallTimer timeIt;

  auto updateDsfStateFn = [&ensemble](const std::shared_ptr<SwitchState>& in) {
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
    utility::populateRemoteIntfAndSysPorts(
        switchId2SystemPorts,
        switchId2Rifs,
        ensemble->getSw()->getConfig(),
        ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    return DsfStateUpdaterUtil::getUpdatedState(
        in,
        ensemble->getSw()->getScopeResolver(),
        ensemble->getSw()->getRib(),
        switchId2SystemPorts,
        switchId2Rifs);
  };
  ensemble->getSw()->getRib()->updateStateInRibThread(
      [&ensemble, updateDsfStateFn]() {
        ensemble->getSw()->updateStateWithHwFailureProtection(
            folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
      });

  // Trigger config apply to add remote interface routes as directly connected
  // in RIB. This is to resolve ECMP members pointing to remote nexthops.
  ensemble->applyNewConfig(voqInitialConfig(*ensemble));

  utility::EcmpSetupTargetedPorts6 ecmpHelper(ensemble->getProgrammedState());
  auto portDescriptor = utility::resolveRemoteNhops(ensemble.get(), ecmpHelper);

  // Measure the time of programming 8x512 wide ECMP
  std::vector<RoutePrefixV6> prefixes;
  std::vector<flat_set<PortDescriptor>> nhopSets;
  for (int i = 0; i < kEcmpGroup; i++) {
    prefixes.push_back(RoutePrefixV6{
        folly::IPAddressV6(folly::to<std::string>(i, "::", i)),
        static_cast<uint8_t>(i == 0 ? 0 : 128)});
    nhopSets.push_back(flat_set<PortDescriptor>(
        std::make_move_iterator(portDescriptor.begin() + i),
        std::make_move_iterator(portDescriptor.begin() + i + kEcmpWidth)));
  }

  auto programRoutes = [&]() {
    auto updater = ensemble->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&updater, nhopSets, prefixes);
  };
  auto unprogramRoutes = [&]() {
    auto updater = ensemble->getSw()->getRouteUpdater();
    ecmpHelper.unprogramRoutes(&updater, prefixes);
  };
  const auto kVoqRouteIteration = 10;
  for (int i = 0; i < kVoqRouteIteration; ++i) {
    if (add) {
      suspender.dismiss();
      programRoutes();
      suspender.rehire();
      unprogramRoutes();
    } else {
      programRoutes();
      suspender.dismiss();
      unprogramRoutes();
      suspender.rehire();
    }
  }
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
