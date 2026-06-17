// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <algorithm>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
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

// MySID SID offset: keeps the /48 MySID entries
// (3001:db8:<i+kMySidLocatorBase>::, built by
// utility::makeAdjacencyMySidEntries) clear of the SRv6 tunnel SID range
// produced by makeSid (3001:db8:1:: .. 3001:db8:1f::), since this benchmark
// programs both in one switch.
constexpr int kMySidLocatorBase = 0x100;

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

// Add numGroups SRv6 ECMP-group routes (2800:<group>::/48), each with
// membersPerGroup unique SRv6 next hops. SIDs are numbered from sidBase so
// multiple callers can keep disjoint SID ranges. The caller programs the
// updater. Returns the programmed prefixes for later deletion.
std::vector<RoutePrefix<folly::IPAddressV6>> addSrv6EcmpGroupRoutes(
    SwSwitchRouteUpdateWrapper& updater,
    const utility::EcmpSetupAnyNPorts6& ecmpHelper,
    int numNhops,
    int numGroups,
    int membersPerGroup,
    int sidBase = 0) {
  std::vector<RoutePrefix<folly::IPAddressV6>> prefixes;
  prefixes.reserve(numGroups);
  for (int group = 0; group < numGroups; ++group) {
    // Build members in a vector and range-construct the flat_set once (avoids
    // O(n^2) single-element inserts into RouteNextHopSet).
    std::vector<ResolvedNextHop> members;
    members.reserve(membersPerGroup);
    for (int member = 0; member < membersPerGroup; ++member) {
      auto globalIndex = group * membersPerGroup + member;
      auto nhop = ecmpHelper.nhop(globalIndex % numNhops);
      members.emplace_back(
          nhop.ip,
          nhop.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{makeSid(sidBase + globalIndex)},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0"));
    }
    RouteNextHopSet nhops(members.begin(), members.end());
    RoutePrefix<folly::IPAddressV6> prefix(
        folly::IPAddressV6(fmt::format("2800:{:x}::", group)), 48);
    updater.addRoute(
        RouterID(0),
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    prefixes.push_back(prefix);
  }
  return prefixes;
}

// Build a shared set of numShared SRv6 next hops (v4 or v6 underlay helper).
// SIDs are numbered from sidBase.
template <typename EcmpHelperT>
RouteNextHopSet makeSharedSrv6Nhops(
    const EcmpHelperT& ecmpHelper,
    int numNhops,
    int numShared,
    int sidBase = 0) {
  std::vector<ResolvedNextHop> members;
  members.reserve(numShared);
  for (int i = 0; i < numShared; ++i) {
    auto nhop = ecmpHelper.nhop(i % numNhops);
    members.emplace_back(
        nhop.ip,
        nhop.intf,
        ECMP_WEIGHT,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::vector<folly::IPAddressV6>{makeSid(sidBase + i)},
        TunnelType::SRV6_ENCAP,
        std::string("srv6Tunnel0"));
  }
  return RouteNextHopSet(members.begin(), members.end());
}

// Add every prefix in |prefixes| pointing at the shared |nhops| set.
template <typename AddrT>
void addSharedSrv6Routes(
    SwSwitchRouteUpdateWrapper& updater,
    const std::vector<RoutePrefix<AddrT>>& prefixes,
    const RouteNextHopSet& nhops) {
  for (const auto& prefix : prefixes) {
    updater.addRoute(
        RouterID(0),
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
  }
}

// Delete every prefix in |prefixes|.
template <typename AddrT>
void delSrv6Routes(
    SwSwitchRouteUpdateWrapper& updater,
    const std::vector<RoutePrefix<AddrT>>& prefixes) {
  for (const auto& prefix : prefixes) {
    updater.delRoute(
        RouterID(0),
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD);
  }
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

  std::vector<RoutePrefix<folly::IPAddressV6>> prefixes;
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    prefixes = addSrv6EcmpGroupRoutes(
        routeUpdater, ecmpHelper, numNhops, numGroups, membersPerGroup);
    routeUpdater.program();
  }
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    delSrv6Routes<folly::IPAddressV6>(routeUpdater, prefixes);
    routeUpdater.program();
  }

  suspender.rehire();
}

// One bucket of a prefix-length distribution: maskLen with a relative weight.
struct PrefixLenWeight {
  uint8_t maskLen;
  uint32_t weight;
};

// Prod backbone IPv6 prefix-length distribution (source: paste P2347358335,
// a backbone device route table). Weights are the raw per-maskLen route counts
// and are scaled to the requested route count at runtime.
const std::vector<PrefixLenWeight> kSrv6V6PrefixDistribution = {
    {60, 7712}, {127, 2862}, {64, 2470}, {56, 1615}, {128, 1363},
    {52, 421},  {48, 407},   {47, 137},  {37, 133},  {55, 66},
    {61, 41},   {62, 22},    {112, 13},  {44, 12},   {39, 9},
    {36, 6},    {46, 4},     {49, 4},    {54, 3},    {32, 2},
    {45, 2},    {104, 1},    {96, 1},    {10, 1},    {40, 1}};

// IPv4 distribution extrapolated from the prod IPv6 prefixes: each IPv6 maskLen
// is mapped
// to a v4 role-equivalent length (aggregate /60->/24, p2p /127->/31, subnet
// /64->/26, /56->/28, host /128->/32, etc.) and weights aggregated, preserving
// the same proportions. Prod data is IPv6-only so v4 has no direct measurement.
const std::vector<PrefixLenWeight> kSrv6V4PrefixDistribution = {
    {24, 7712},
    {31, 2862},
    {26, 2470},
    {28, 1615},
    {32, 1377},
    {20, 544},
    {22, 421},
    {16, 151},
    {23, 69},
    {25, 63},
    {18, 14},
    {15, 6},
    {19, 4},
    {21, 4},
    {14, 2},
    {8, 1}};

// Scale a weighted distribution to exactly |target| prefixes, using the
// largest-remainder method so the bucket counts sum to target.
std::vector<PrefixLenWeight> scaleDistribution(
    const std::vector<PrefixLenWeight>& dist,
    uint32_t target) {
  std::vector<PrefixLenWeight> scaled;
  if (dist.empty() || target == 0) {
    return scaled;
  }
  uint64_t totalWeight = 0;
  for (const auto& bucket : dist) {
    totalWeight += bucket.weight;
  }
  CHECK_GT(totalWeight, 0u);
  std::vector<std::pair<double, size_t>> remainders;
  scaled.reserve(dist.size());
  remainders.reserve(dist.size());
  uint32_t assigned = 0;
  for (size_t i = 0; i < dist.size(); ++i) {
    double exact = static_cast<double>(dist[i].weight) * target / totalWeight;
    auto floorCount = static_cast<uint32_t>(exact);
    scaled.push_back({dist[i].maskLen, floorCount});
    assigned += floorCount;
    remainders.emplace_back(exact - floorCount, i);
  }
  // Hand the remaining routes to the buckets with the largest fractional parts.
  std::sort(
      remainders.begin(), remainders.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
      });
  for (const auto& [frac, idx] : remainders) {
    if (assigned >= target) {
      break;
    }
    CHECK_LT(idx, scaled.size());
    scaled[idx].weight += 1;
    ++assigned;
  }
  return scaled;
}

// OR a base prefix into the network portion of |addr|. The generated prefixes
// occupy only the low/network bits per bucket, leaving the top hextet/octet
// free for the base so routes don't collide with interface/loopback subnets.
template <typename AddrT>
AddrT relocateUnderBase(const AddrT& addr, const AddrT& base) {
  auto bytes = addr.toByteArray();
  const auto baseBytes = base.toByteArray();
  for (size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] |= baseBytes[i];
  }
  return AddrT(bytes);
}

// Generate distinct prefixes following a scaled prefix-length distribution.
// A fresh PrefixGenerator per bucket yields collision-free prefixes within a
// length; across lengths prefixes differ by mask, so every route is unique.
template <typename AddrT>
std::vector<RoutePrefix<AddrT>> genDistributedPrefixes(
    const std::vector<PrefixLenWeight>& scaled,
    const AddrT& base) {
  std::vector<RoutePrefix<AddrT>> prefixes;
  for (const auto& [maskLen, count] : scaled) {
    utility::PrefixGenerator<AddrT> gen(maskLen);
    for (uint32_t i = 0; i < count; ++i) {
      auto prefix = gen.getNext();
      prefixes.emplace_back(
          relocateUnderBase<AddrT>(prefix.network(), base), maskLen);
    }
  }
  return prefixes;
}

// Benchmark: route scale with shared SRv6 nhops.
// Programs numV6Routes IPv6 and numV4Routes IPv4 routes following a
// prod-derived prefix-length distribution (see kSrv6V6/V4PrefixDistribution),
// all pointing to a shared set of SRv6 next hops (reusing the same SID lists).
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
  auto sharedV6Nhops =
      makeSharedSrv6Nhops(ecmpHelper6, numNhops, kNumSharedSrv6Nhops);
  auto sharedV4Nhops =
      makeSharedSrv6Nhops(ecmpHelper4, numNhops, kNumSharedSrv6Nhops);

  // Generate prod-like prefixes once (untimed) and reuse the same vectors for
  // both add and delete — keeps add/delete in lockstep with no duplicated
  // prefix formula. Routes are based under 2800:: / 100.0.0.0 to avoid the
  // interface/loopback subnets.
  auto v6Prefixes = genDistributedPrefixes<folly::IPAddressV6>(
      scaleDistribution(kSrv6V6PrefixDistribution, numV6Routes),
      folly::IPAddressV6("2800::"));
  auto v4Prefixes = genDistributedPrefixes<folly::IPAddressV4>(
      scaleDistribution(kSrv6V4PrefixDistribution, numV4Routes),
      folly::IPAddressV4("100.0.0.0"));

  // Timed: program and unprogram all routes in one shot
  suspender.dismiss();

  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    addSharedSrv6Routes<folly::IPAddressV6>(
        routeUpdater, v6Prefixes, sharedV6Nhops);
    routeUpdater.program();
  }
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    addSharedSrv6Routes<folly::IPAddressV4>(
        routeUpdater, v4Prefixes, sharedV4Nhops);
    routeUpdater.program();
  }
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    delSrv6Routes<folly::IPAddressV6>(routeUpdater, v6Prefixes);
    routeUpdater.program();
  }
  {
    auto routeUpdater = ensemble->getSw()->getRouteUpdater();
    delSrv6Routes<folly::IPAddressV4>(routeUpdater, v4Prefixes);
    routeUpdater.program();
  }

  suspender.rehire();
}

// Full-scale SRv6 warmboot benchmark: program numGroups x membersPerGroup
// unique SRv6 next hops, numV6Routes v6 routes (shared SRv6 nhops), and
// numMySidEntries ADJACENCY_MICRO_SID MySID entries, then measure the warmboot
// init-to-CONFIGURED time (folly). The binary is run twice: a coldboot pass
// with
// --setup_for_warmboot writes the state, a warmboot pass restores it and times
// init.
void srv6FullScaleWarmbootBenchmark(
    int numGroups,
    int membersPerGroup,
    int numV6Routes,
    int numMySidEntries) {
  folly::BenchmarkSuspender suspender;
  constexpr int kNumSharedSrv6Nhops = 8;
  FLAGS_ecmp_resource_percentage = 100;
  // Don't cap MySID usage at the default percentage (match the MySID
  // benchmark).
  FLAGS_enable_mysid_resource_protection = true;
  FLAGS_mysid_resource_percentage = 100;

  // Timed: init to CONFIGURED. On the warmboot pass this restores the saved
  // nhops + routes into hardware.
  suspender.dismiss();
  auto ensemble = createAgentEnsemble(
      makeSrv6ConfigFn(), false /*disableLinkStateToggler*/);
  suspender.rehire();

  // Coldboot only: build the combined SRv6 state so gracefulExit persists it to
  // the warmboot dir. On warmboot it is already restored above.
  if (ensemble->getBootType() == BootType::COLD_BOOT) {
    ensemble->applyNewState(
        [](const std::shared_ptr<SwitchState>& in) {
          return utility::enableTrunkPorts(in);
        },
        "enable trunk ports");

    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        ensemble->getSw()->getState(),
        ensemble->getSw()->needL2EntryForNeighbor());
    auto numNhops =
        std::min(static_cast<int>(ecmpHelper.getNextHops().size()), 64);
    CHECK_GT(numNhops, 0);
    ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, numNhops);
    });

    auto routeUpdater = ensemble->getSw()->getRouteUpdater();

    // numGroups routes, each with membersPerGroup unique SRv6 nhops (SIDs
    // 0..numGroups*membersPerGroup-1, base 2800::).
    addSrv6EcmpGroupRoutes(
        routeUpdater, ecmpHelper, numNhops, numGroups, membersPerGroup);

    // numV6Routes routes sharing one SRv6 nhop set. SIDs start after the group
    // SIDs; base under 2a00:: to avoid colliding with the 2800::/48 group
    // routes above.
    auto sharedV6Nhops = makeSharedSrv6Nhops(
        ecmpHelper, numNhops, kNumSharedSrv6Nhops, numGroups * membersPerGroup);
    auto v6Prefixes = genDistributedPrefixes<folly::IPAddressV6>(
        scaleDistribution(kSrv6V6PrefixDistribution, numV6Routes),
        folly::IPAddressV6("2a00::"));
    addSharedSrv6Routes<folly::IPAddressV6>(
        routeUpdater, v6Prefixes, sharedV6Nhops);
    routeUpdater.program();

    // numMySidEntries ADJACENCY_MICRO_SID MySID entries (separate my_sid
    // table). sidOffset keeps the /48 SIDs clear of the SRv6 tunnel SID range.
    utility::programMySidEntries(
        ensemble->getSw(),
        utility::makeAdjacencyMySidEntries(
            ecmpHelper, numNhops, numMySidEntries, kMySidLocatorBase));
  }

  ensemble->stopStatsThread();
  if (FLAGS_setup_for_warmboot) {
    // Coldboot setup pass: persist the warmboot state so the warmboot pass can
    // reload it, then leak the ensemble so destructors don't unprogram
    // hardware. The static StopWatch times the graceful exit (warm_boot_msecs),
    // i.e. the warmboot state-persist time; its dtor fires at process exit.
    static StopWatch timer("warm_boot_msecs", FLAGS_json);
    ensemble->gracefulExit();
    __attribute__((unused)) auto leakedHwEnsemble = ensemble.release();
  }
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

// ASIC supports 50K routes with SRv6 encap. Routes follow a prod backbone
// prefix-length distribution (P2347358335). Test all three flavors:
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

// Full-scale warmboot: 390 groups x 20 = 7.8K SRv6 next hops + 50K v6 routes +
// 2042 ADJACENCY_MICRO_SID MySID entries.
BENCHMARK(HwSrv6FullScaleWarmbootBenchmark) {
  srv6FullScaleWarmbootBenchmark(390, 20, 50000, 2042);
}

} // namespace facebook::fboss
