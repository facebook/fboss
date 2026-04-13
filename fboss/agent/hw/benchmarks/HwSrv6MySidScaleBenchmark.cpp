// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
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

  // Timed: program MySID entries one at a time
  suspender.dismiss();
  for (int i = 0; i < numEntries; ++i) {
    MySidEntry entry;
    entry.type() = MySidType::ADJACENCY_MICRO_SID;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() = facebook::network::toBinaryAddress(
        folly::IPAddressV6(makeMySidAddr(i)));
    prefix.prefixLength() = kMySidPrefixLen;
    entry.mySid() = prefix;
    NextHopThrift nhop;
    nhop.address() =
        facebook::network::toBinaryAddress(ecmpHelper.nhop(i % numNhops).ip);
    entry.nextHops()->push_back(nhop);
    rib->update(
        sw->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addMySidEntry",
        ribMySidFunc,
        sw);
  }
  suspender.rehire();

  // Cleanup: delete all MySID entries (untimed)
  for (int i = 0; i < numEntries; ++i) {
    IpPrefix prefix;
    prefix.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6(makeMySidAddr(i)));
    prefix.prefixLength() = kMySidPrefixLen;
    rib->update(
        sw->getScopeResolver(),
        {} /* toAdd */,
        {prefix},
        "deleteMySidEntry",
        ribMySidFunc,
        sw);
  }
}

} // namespace

// ASIC supports up to 2K MySID entries.
// Midpoint (ADJACENCY_MICRO_SID): each entry has a next hop for uSID shift
// and forwarding to the adjacency.
BENCHMARK(HwSrv6MidpointMySidScaleBenchmark) {
  srv6MidpointMySidScaleBenchmark(2048);
}

} // namespace facebook::fboss
