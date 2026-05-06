// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"

namespace facebook::fboss {

namespace {

// Generate a unique SID address from a global index.
// Format: 3001:db8:XX:YY:: where XX = (index >> 8) + 1, YY = index & 0xFF
folly::IPAddressV6 makeSid(int index) {
  return folly::IPAddressV6(
      fmt::format("3001:db8:{:x}:{:x}::", (index >> 8) + 1, index & 0xFF));
}

constexpr int kPortsPerLag = 2;

AgentEnsembleSwitchConfigFn makeSrv6ConfigFn() {
  return [](const AgentEnsemble& ensemble) {
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    // Ensure even number of ports for 2-port LAGs
    auto allPorts = ensemble.masterLogicalPortIds();
    auto numPorts = (allPorts.size() / kPortsPerLag) * kPortsPerLag;
    std::vector<PortID> portIds(allPorts.begin(), allPorts.begin() + numPorts);
    // 2 ports per interface — each interface maps to one LAG
    auto cfg = utility::multiplePortsPerIntfConfig(
        ensemble.getPlatformMapping(),
        asic,
        portIds,
        ensemble.supportsAddRemovePort(),
        utility::kDefaultLoopbackMap(),
        true /*interfaceHasSubnet*/,
        true /*setInterfaceMac*/,
        utility::kBaseVlanId,
        kPortsPerLag);
    cfg.srv6Tunnels() = {utility::makeSrv6TunnelConfig(
        "srv6Tunnel0", InterfaceID(cfg.interfaces()[0].intfID().value()))};
    // RBB interfaces are always LAG bundles — 2-port LAGs
    for (int i = 0; i + 1 < portIds.size(); i += kPortsPerLag) {
      utility::addAggPort(
          i / kPortsPerLag + 1,
          {static_cast<int32_t>(portIds[i]),
           static_cast<int32_t>(portIds[i + 1])},
          &cfg);
    }
    return cfg;
  };
}

// Benchmark: ECMP group scale with unique SRv6 nhops per group.
// Programs numGroups routes, each with membersPerGroup SRv6 next hops.
void srv6EcmpGroupScaleBenchmark(int numGroups, int membersPerGroup) {
  folly::BenchmarkSuspender suspender;

  FLAGS_ecmp_resource_percentage = 100;
  auto ensemble = createAgentEnsemble(
      makeSrv6ConfigFn(), false /*disableLinkStateToggler*/);
  ensemble->applyNewState(
      [](const std::shared_ptr<SwitchState>& in) {
        return utility::enableTrunkPorts(in);
      },
      "enable trunk ports");

  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  // Use ecmpHelper's nhop count — with 2-port LAGs, deduplicated nhops
  // are fewer than physical ports
  auto numNhops =
      std::min(static_cast<int>(ecmpHelper.getNextHops().size()), 64);
  CHECK_GT(numNhops, 0);

  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, numNhops);
  });

  // Timed: program and unprogram all routes in one shot
  suspender.dismiss();

  // Program all routes with unique SRv6 nhops per group
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int group = 0; group < numGroups; ++group) {
      RouteNextHopSet nhops;
      for (int member = 0; member < membersPerGroup; ++member) {
        auto globalIndex = group * membersPerGroup + member;
        auto nhop = ecmpHelper.nhop(globalIndex % numNhops);
        nhops.insert(ResolvedNextHop(
            nhop.ip,
            nhop.intf,
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{makeSid(globalIndex)},
            TunnelType::SRV6_ENCAP,
            std::string("srv6Tunnel0")));
      }
      routeUpdater.addRoute(
          RouterID(0),
          folly::IPAddressV6(fmt::format("2800:{:x}::", group)),
          48,
          ClientID::BGPD,
          RouteNextHopEntry(nhops, AdminDistance::EBGP));
    }
    routeUpdater.program();
  }

  // Unprogram all routes
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int group = 0; group < numGroups; ++group) {
      routeUpdater.delRoute(
          RouterID(0),
          folly::IPAddress(
              folly::IPAddressV6(fmt::format("2800:{:x}::", group))),
          48,
          ClientID::BGPD);
    }
    routeUpdater.program();
  }

  suspender.rehire();
}

// Benchmark: route scale with shared SRv6 nhops.
// Programs numV6Routes IPv6 and numV4Routes IPv4 routes, all pointing to
// a shared set of SRv6 next hops (reusing the same SID lists).
void srv6RouteScaleBenchmark(int numV6Routes, int numV4Routes) {
  folly::BenchmarkSuspender suspender;
  constexpr int kNumSharedSrv6Nhops = 8;

  auto ensemble = createAgentEnsemble(
      makeSrv6ConfigFn(), false /*disableLinkStateToggler*/);
  ensemble->applyNewState(
      [](const std::shared_ptr<SwitchState>& in) {
        return utility::enableTrunkPorts(in);
      },
      "enable trunk ports");

  utility::EcmpSetupAnyNPorts6 ecmpHelper6(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  utility::EcmpSetupAnyNPorts4 ecmpHelper4(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  // Use ecmpHelper's nhop count — with 2-port LAGs, deduplicated nhops
  // are fewer than physical ports
  auto numNhops =
      std::min(static_cast<int>(ecmpHelper6.getNextHops().size()), 64);
  CHECK_GT(numNhops, 0);

  // Resolve physical next hops for both v4 and v6
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper6.resolveNextHops(in, numNhops);
  });
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper4.resolveNextHops(in, numNhops);
  });

  // Build shared SRv6 next hop sets (untimed)
  RouteNextHopSet sharedV6Nhops;
  for (int i = 0; i < kNumSharedSrv6Nhops; ++i) {
    auto nhop = ecmpHelper6.nhop(i % numNhops);
    sharedV6Nhops.insert(ResolvedNextHop(
        nhop.ip,
        nhop.intf,
        ECMP_WEIGHT,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::vector<folly::IPAddressV6>{makeSid(i)},
        TunnelType::SRV6_ENCAP,
        std::string("srv6Tunnel0")));
  }

  RouteNextHopSet sharedV4Nhops;
  for (int i = 0; i < kNumSharedSrv6Nhops; ++i) {
    auto nhop = ecmpHelper4.nhop(i % numNhops);
    sharedV4Nhops.insert(ResolvedNextHop(
        nhop.ip,
        nhop.intf,
        ECMP_WEIGHT,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::vector<folly::IPAddressV6>{makeSid(i)},
        TunnelType::SRV6_ENCAP,
        std::string("srv6Tunnel0")));
  }

  // Timed: program and unprogram all routes in one shot
  suspender.dismiss();

  // Program all IPv6 routes
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int i = 0; i < numV6Routes; ++i) {
      routeUpdater.addRoute(
          RouterID(0),
          folly::IPAddressV6(
              fmt::format("2800:{:x}:{:x}::", (i >> 8) & 0xFFFF, i & 0xFF)),
          64,
          ClientID::BGPD,
          RouteNextHopEntry(sharedV6Nhops, AdminDistance::EBGP));
    }
    routeUpdater.program();
  }

  // Program all IPv4 routes
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int i = 0; i < numV4Routes; ++i) {
      auto byte1 = ((i >> 8) & 0xFF) + 1;
      auto byte2 = i & 0xFF;
      routeUpdater.addRoute(
          RouterID(0),
          folly::IPAddressV4(fmt::format("100.{}.{}.0", byte1, byte2)),
          24,
          ClientID::BGPD,
          RouteNextHopEntry(sharedV4Nhops, AdminDistance::EBGP));
    }
    routeUpdater.program();
  }

  // Delete all IPv6 routes
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int i = 0; i < numV6Routes; ++i) {
      routeUpdater.delRoute(
          RouterID(0),
          folly::IPAddress(
              folly::IPAddressV6(
                  fmt::format(
                      "2800:{:x}:{:x}::", (i >> 8) & 0xFFFF, i & 0xFF))),
          64,
          ClientID::BGPD);
    }
    routeUpdater.program();
  }

  // Delete all IPv4 routes
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    for (int i = 0; i < numV4Routes; ++i) {
      auto byte1 = ((i >> 8) & 0xFF) + 1;
      auto byte2 = i & 0xFF;
      routeUpdater.delRoute(
          RouterID(0),
          folly::IPAddress(
              folly::IPAddressV4(fmt::format("100.{}.{}.0", byte1, byte2))),
          24,
          ClientID::BGPD);
    }
    routeUpdater.program();
  }

  suspender.rehire();
}

} // namespace

// ASIC supports up to 7.8K SRv6 next hops: 390 groups x 20 members = 7800
BENCHMARK(HwSrv6NextHopScaleBenchmark) {
  srv6EcmpGroupScaleBenchmark(390, 20);
}

// Current SDK release supports up to 1K ECMP groups: 1024 groups x 5 members
BENCHMARK(HwSrv6EcmpGroupScaleBenchmark) {
  srv6EcmpGroupScaleBenchmark(1024, 5);
}

// Single-nhop SRv6 route scale: 3000 routes, each with its own unique SRv6
// next hop (3000 distinct SidList + underlay-nhop SAI objects).
BENCHMARK(HwSrv6SingleNextHopRouteScaleBenchmark) {
  srv6EcmpGroupScaleBenchmark(3000, 1);
}

// ASIC supports 50K routes with SRv6 encap. Test all three flavors:
// 1. Only IPv6 routes
BENCHMARK(HwSrv6V6RouteScaleBenchmark) {
  srv6RouteScaleBenchmark(50000, 0);
}

// 2. Only IPv4 routes
BENCHMARK(HwSrv6V4RouteScaleBenchmark) {
  srv6RouteScaleBenchmark(0, 50000);
}

// 3. Mixed IPv6 + IPv4 routes
BENCHMARK(HwSrv6MixedRouteScaleBenchmark) {
  srv6RouteScaleBenchmark(25000, 25000);
}

} // namespace facebook::fboss
