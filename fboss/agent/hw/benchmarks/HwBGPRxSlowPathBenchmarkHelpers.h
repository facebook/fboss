#include "fboss/agent/TxPacket.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PortFlapHelper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/StressTestUtils.h"

namespace {
const std::string kDstIp = "2620:0:1cfe:face:b00c::4";
}; // namespace

namespace facebook::fboss::utility {
enum BgpRxMode { routeProgramming, routeProgrammingWithPortFlap };

void rxSlowPathBGPRouteChangeBenchmark(BgpRxMode mode) {
  auto ensemble = createAgentEnsemble(
      bgpRxBenchmarkConfig, false /*disableLinkStateToggler*/);

  resolveNhopForRouteGenerator<utility::FSWRouteScaleGenerator>(ensemble.get());

  // capture packet exiting port 0 (entering due to loopback)
  auto dstMac = utility::getFirstInterfaceMac(ensemble->getProgrammedState());
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getProgrammedState(), dstMac);
  flat_set<PortDescriptor> IntfPorts;
  IntfPorts.insert(
      PortDescriptor(ensemble->masterLogicalInterfacePortIds()[0]));

  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, IntfPorts);
  });
  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      IntfPorts,
      {RoutePrefixV6{folly::IPAddressV6(), 0}});
  // Disable TTL decrements
  utility::ttlDecrementHandlingForLoopbackTraffic(
      ensemble.get(), ecmpHelper.getRouterId(), ecmpHelper.getNextHops()[0]);

  const auto kSrcMac = folly::MacAddress{"fa:ce:b0:00:00:0c"};
  // Send packet
  auto vlanId = utility::firstVlanID(ensemble->getProgrammedState());
  auto constexpr kPacketToSend = 10;
  for (int i = 0; i < kPacketToSend; i++) {
    auto txPacket = utility::makeTCPTxPacket(
        ensemble->getSw(),
        vlanId,
        kSrcMac,
        dstMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6(kDstIp),
        47231,
        kBgpPort);
    ensemble->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }

  constexpr auto kBurnIntevalInSeconds = 10;
  // Let the packet flood warm up
  std::this_thread::sleep_for(std::chrono::seconds(kBurnIntevalInSeconds));
  // start baseline BGP benchmark test
  constexpr uint8_t kCpuQueue = 0;
  // start port flap
  std::unique_ptr<PortFlapHelper> portFlapHelper;
  if (mode == routeProgrammingWithPortFlap) {
    // first 4 ECMP port, skip the traffic port
    CHECK_GE(ensemble->masterLogicalInterfacePortIds().size(), 5);
    std::vector<PortID> portToFlap;
    for (auto i = 0; i < 4; i++) {
      portToFlap.push_back(ensemble->masterLogicalInterfacePortIds()[1 + i]);
    }
    portFlapHelper =
        std::make_unique<PortFlapHelper>(ensemble.get(), portToFlap);
  }
  std::map<int, CpuPortStats> cpuStatsBefore;
  ensemble->getSw()->getAllCpuPortStats(cpuStatsBefore);
  auto statsBefore = cpuStatsBefore[0];
  auto [pktsBefore, bytesBefore] = utility::getCpuQueueOutPacketsAndBytes(
      *statsBefore.portStats_(), kCpuQueue);
  auto timeBefore = std::chrono::steady_clock::now();
  CHECK_NE(pktsBefore, 0);

  if (mode == routeProgrammingWithPortFlap) {
    CHECK(portFlapHelper);
    portFlapHelper->startPortFlap();
  }

  auto [bgpRouteAddWorstCaseLookupMsecs, bgpRouteAddWorstCaseBulkLookupMsecs] =
      routeChangeLookupStresser<utility::FSWRouteScaleGenerator>(
          ensemble.get());

  std::map<int, CpuPortStats> cpuStatsAfter;
  ensemble->getSw()->getAllCpuPortStats(cpuStatsAfter);
  auto statsAfter = cpuStatsAfter[0];
  auto [pktsAfter, bytesAfter] = utility::getCpuQueueOutPacketsAndBytes(
      *statsAfter.portStats_(), kCpuQueue);
  auto timeAfter = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> durationMillseconds =
      timeAfter - timeBefore;
  uint32_t bgpRouteAddPPS = (static_cast<double>(pktsAfter - pktsBefore) /
                             durationMillseconds.count()) *
      1000;
  uint32_t bgpRouteAddBytesPerSec =
      (static_cast<double>(bytesAfter - bytesBefore) /
       durationMillseconds.count()) *
      1000;

  if (mode == routeProgrammingWithPortFlap) {
    CHECK(portFlapHelper);
    portFlapHelper->stopPortFlap();
  }

  if (FLAGS_json) {
    folly::dynamic cpuRxRateJson = folly::dynamic::object;
    cpuRxRateJson["cpu_rx_pps"] = bgpRouteAddPPS;
    cpuRxRateJson["cpu_rx_bytes_per_sec"] = bgpRouteAddBytesPerSec;
    std::cout << toPrettyJson(cpuRxRateJson) << std::endl;
  } else {
    XLOG(DBG2) << " Pkts before: " << pktsBefore << " Pkts after: " << pktsAfter
               << " interval ms: " << durationMillseconds.count()
               << " pps: " << bgpRouteAddPPS
               << " bytes per sec: " << bgpRouteAddBytesPerSec;
    XLOG(DBG2) << " BGP route add worst case lookup msecs: "
               << bgpRouteAddWorstCaseLookupMsecs
               << " BGP route add worst case bulk lookup msecs: "
               << bgpRouteAddWorstCaseBulkLookupMsecs;
  }
}

} // namespace facebook::fboss::utility
