// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

namespace {

// Generate a unique MySID address from an index.
// The locator block is the first 32 bits (3001:0db8).
// The function ID is the next 16 bits (third group), varied per entry.
folly::IPAddressV6 makeMySidAddr(int index) {
  return folly::IPAddressV6(fmt::format("3001:db8:{:x}::", index + 1));
}

constexpr int kMySidPrefixLen = 48;

AgentEnsembleSwitchConfigFn makeMySidConfigFn() {
  return [](const AgentEnsemble& ensemble) {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
  };
}

// Benchmark: programs numEntries ADJACENCY_MICRO_SID MySID entries
// one at a time and measures the total time to program them to the ASIC.
void srv6MidpointMySidScaleBenchmark(int numEntries) {
  folly::BenchmarkSuspender suspender;
  // For scale benchmarking, do not cap MySID usage at the default 75%.
  // This aligns with other scale benchmarks that set resource percentages to
  // 100.
  FLAGS_enable_mysid_resource_protection = true;
  FLAGS_mysid_resource_percentage = 100;

  auto ensemble = createAgentEnsemble(
      makeMySidConfigFn(), false /*disableLinkStateToggler*/);

  // Resolve next hops for ADJACENCY_MICRO_SID entries
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto numNhops =
      std::min(static_cast<int>(ecmpHelper.getNextHops().size()), 64);
  CHECK_GT(numNhops, 0);

  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, numNhops);
  });

  auto sw = ensemble->getSw();
  auto rib = sw->getRib();
  auto ribMySidFunc = createRibMySidToSwitchStateFunction(std::nullopt);
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark targetEntries="
             << numEntries;

  // Timed: program MySID entries one at a time
  int programmedEntries = 0;
  suspender.dismiss();
  for (int i = 0; i < numEntries; ++i) {
    auto ecmpNhop = ecmpHelper.nhop(i % numNhops);
    state::MySidFields fields;
    fields.type() = MySidType::ADJACENCY_MICRO_SID;
    fields.adjacencyInterfaceId() = static_cast<int32_t>(ecmpNhop.intf);
    // EcmpSetupAnyNPorts6 always yields IPv6 next hops.
    fields.isV6() = true;
    fields.clientId() = ClientID::STATIC_ROUTE;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() = facebook::network::toBinaryAddress(
        folly::IPAddressV6(makeMySidAddr(i)));
    prefix.prefixLength() = kMySidPrefixLen;
    fields.mySid() = prefix;
    auto mySid = std::make_shared<MySid>(fields);
    RouteNextHopSet nhops{ResolvedNextHop(
        folly::IPAddress(ecmpNhop.ip), ecmpNhop.intf, ECMP_WEIGHT)};
    std::vector<MySidWithNextHops> toAdd = {
        {mySid, nhops, std::nullopt /* nextHopGroupName */}};
    rib->update(
        sw->getScopeResolver(),
        std::move(toAdd),
        {} /* toUnresolveIfMatch */,
        {} /* toDelete */,
        "addMySidEntry",
        ribMySidFunc,
        sw);
    ++programmedEntries;
  }
  suspender.rehire();
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark programmedEntries="
             << programmedEntries;

  // Cleanup: delete all MySID entries (untimed)
  for (int i = 0; i < programmedEntries; ++i) {
    IpPrefix prefix;
    prefix.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6(makeMySidAddr(i)));
    prefix.prefixLength() = kMySidPrefixLen;
    rib->update(
        sw->getScopeResolver(),
        std::vector<MySidEntry>{} /* toAdd */,
        {prefix},
        "deleteMySidEntry",
        ribMySidFunc,
        sw);
  }
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark cleanupEntries="
             << programmedEntries;
}

} // namespace

// Current validated capacity is 2042 MySID entries in SAI tests.
// Midpoint (ADJACENCY_MICRO_SID): each entry has a next hop for uSID shift
// and forwarding to the adjacency.
BENCHMARK(HwSrv6MidpointMySidScaleBenchmark) {
  srv6MidpointMySidScaleBenchmark(2042);
}

} // namespace facebook::fboss
