#include "fboss/agent/test/utils/StressTestUtils.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/Benchmark.h>

#include "fboss/agent/IPv6Handler.h"

namespace {
const std::string kDstIp = "2620:0:1cfe:face:b00c::4";
// at least 4 ports to support ECMP
const int kEcmpWidth = 8;
}; // namespace

namespace facebook::fboss::utility {

template <typename RouteScaleGeneratorT>
void resolveNhopForRouteGenerator(AgentEnsemble* ensemble) {
  auto* sw = ensemble->getSw();
  auto routeGenerator = RouteScaleGeneratorT(sw->getState());
  auto swSwitch = ensemble->agentInitializer()->sw();
  auto platformType = swSwitch->getPlatformType();
  if (!routeGenerator.isSupported(platformType)) {
    throw FbossError(
        "Route scale generator not supported for platform: ", platformType);
  }
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return routeGenerator.resolveNextHops(in);
  });
  return;
}

template <typename RouteScaleGeneratorT>
std::tuple<double, double> routeChangeLookupStresser(AgentEnsemble* ensemble) {
  folly::BenchmarkSuspender suspender;

  auto* sw = ensemble->getSw();
  auto routeGenerator = RouteScaleGeneratorT(sw->getState());
  auto swSwitch = ensemble->agentInitializer()->sw();
  auto platformType = swSwitch->getPlatformType();
  if (!routeGenerator.isSupported(platformType)) {
    throw FbossError(
        "Route scale generator not supported for platform: ", platformType);
  }

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

  double worstCaseLookupMsecs, worstCaseBulkLookupMsecs;
  std::atomic<bool> done{false};
  auto doLookups = [&ensemble,
                    kRid,
                    &done,
                    &worstCaseLookupMsecs,
                    &worstCaseBulkLookupMsecs]() {
    worstCaseLookupMsecs = 0;
    worstCaseBulkLookupMsecs = 0;
    auto programmedState = ensemble->getSw()->getState();
    std::vector<folly::IPAddressV6> addrsToLookup;
    utility::IPAddressGenerator<folly::IPAddressV6> ipAddrGen;
    for (auto i = 0; i < 1000; ++i) {
      addrsToLookup.emplace_back(ipAddrGen.getNext());
    }

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
            [ensemble,
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

    XLOG(DBG2) << "worst_case_lookup_msescs : " << worstCaseLookupMsecs
               << " worst_case_bulk_lookup_msescs : "
               << worstCaseBulkLookupMsecs;
  };
  // Start parallel lookup thread
  std::thread lookupThread([&doLookups]() { doLookups(); });
  auto updater = ensemble->getSw()->getRouteUpdater();
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
      [&updater, kRid](
          const utility::RouteDistributionGenerator::ThriftRouteChunk& routes) {
        std::for_each(
            routes.begin(), routes.end(), [&updater, kRid](const auto& route) {
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
  done = true;
  lookupThread.join();
  return std::make_tuple(worstCaseLookupMsecs, worstCaseBulkLookupMsecs);
}
template std::tuple<double, double> routeChangeLookupStresser<
    utility::FSWRouteScaleGenerator>(AgentEnsemble* ensemble);
template void resolveNhopForRouteGenerator<FSWRouteScaleGenerator>(
    AgentEnsemble* ensemble);

cfg::SwitchConfig bgpRxBenchmarkConfig(const AgentEnsemble& ensemble) {
  FLAGS_sai_user_defined_trap = true;
  CHECK_GE(
      ensemble.masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(), 1);

  std::vector<PortID> ports;
  // ECMP needs at least 4 data interfaces
  for (auto i = 0; i < kEcmpWidth; i++)
    ports.push_back(ensemble.masterLogicalInterfacePortIds()[i]);

  // For J2 and J3, initialize recycle port as well to allow l3 lookup on
  // recycle port
  if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT)) {
    ports.push_back(
        ensemble.masterLogicalPortIds({cfg::PortType::RECYCLE_PORT})[0]);
  }
  auto config = utility::onePortPerInterfaceConfig(ensemble.getSw(), ports);
  utility::setupDefaultAclTableGroups(config);
  // We don't want to set queue rate that limits the number of rx pkts
  utility::addCpuQueueConfig(
      config,
      ensemble.getL3Asics(),
      ensemble.isSai(),
      /* setQueueRate */ false);
  auto asic = utility::checkSameAndGetAsic(ensemble.getL3Asics());
  auto trapDstIp = folly::CIDRNetwork{kDstIp, 128};
  utility::addTrapPacketAcl(asic, &config, trapDstIp);

  // Since J2 and J3 does not support disabling TLL on port, create TRAP to
  // forward TTL=0 packet. Also not send icmp time exceeded packet, since CPU
  // will trap TTL=0 packet.
  if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT)) {
    FLAGS_send_icmp_time_exceeded = false;
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
  }
  return config;
}

} // namespace facebook::fboss::utility
