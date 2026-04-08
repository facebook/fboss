// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
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
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
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

  // Timed: program all routes with unique SRv6 nhops per group
  constexpr int kBatchSize = 64;
  suspender.dismiss();
  for (int batch = 0; batch < numGroups; batch += kBatchSize) {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    auto batchEnd = std::min(batch + kBatchSize, numGroups);
    for (int group = batch; group < batchEnd; ++group) {
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
  suspender.rehire();

  // Cleanup: unprogram all routes (untimed)
  for (int batch = 0; batch < numGroups; batch += kBatchSize) {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    auto batchEnd = std::min(batch + kBatchSize, numGroups);
    for (int group = batch; group < batchEnd; ++group) {
      routeUpdater.delRoute(
          RouterID(0),
          folly::IPAddress(
              folly::IPAddressV6(fmt::format("2800:{:x}::", group))),
          48,
          ClientID::BGPD);
    }
    routeUpdater.program();
  }
}

} // namespace

// ASIC supports up to 8K SRv6 next hops: 400 groups x 20 members = 8000
BENCHMARK(HwSrv6NextHopScaleBenchmark) {
  srv6EcmpGroupScaleBenchmark(400, 20);
}

// Current SDK release supports up to 1K ECMP groups: 1024 groups x 5 members
BENCHMARK(HwSrv6EcmpGroupScaleBenchmark) {
  srv6EcmpGroupScaleBenchmark(1024, 5);
}

} // namespace facebook::fboss
