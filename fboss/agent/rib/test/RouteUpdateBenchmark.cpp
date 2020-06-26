/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "common/init/Init.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/Benchmark.h>

using namespace facebook::fboss;

auto constexpr kEcmpWidth = 4;

namespace {

facebook::network::thrift::BinaryAddress createNextHop(
    const folly::IPAddress& addr) {
  auto addrAsBinaryAddress = facebook::network::toBinaryAddress(addr);
  addrAsBinaryAddress.ifName_ref() = "fboss1";
  return addrAsBinaryAddress;
}

std::vector<NextHopThrift> nextHopsThrift(
    const std::vector<folly::IPAddress>& addrs) {
  std::vector<NextHopThrift> nexthops;
  for (const auto& addr : addrs) {
    NextHopThrift nexthop;
    *nexthop.address_ref() = createNextHop(addr);
    *nexthop.weight_ref() = static_cast<int32_t>(ECMP_WEIGHT);
    nexthops.emplace_back(std::move(nexthop));
  }
  return nexthops;
}

} // namespace

template <typename Generator>
static void runOldRibTest() {
  // Suspend benchamrking for setup.
  folly::BenchmarkSuspender suspender;

  SimPlatform plat(folly::MacAddress(), 128);
  std::vector<PortID> ports;
  for (int i = 0; i < 128; ++i) {
    ports.push_back(PortID(i));
  }
  cfg::SwitchConfig config =
      utility::onePortPerVlanConfig(plat.getHwSwitch(), ports);
  auto testHandle = createTestHandle(&config, SwitchFlags::DEFAULT);
  auto sw = testHandle->getSw();

  auto generator = Generator(sw->getAppliedState(), 1337, kEcmpWidth);

  // Generate the route chunks which are then cached in the generator and
  // reused when we call `getSwitchStates()`.
  generator.get();

  // Resume benchmakring post-setup.
  suspender.dismiss();

  auto states = generator.getSwitchStates();

  for (const auto& state : states) {
    sw->updateStateBlocking(
        "route updates",
        [=](const std::shared_ptr<SwitchState>& /* oldState */) {
          return state;
        });
  }
}

template <typename Generator>
static void runNewRibTest() {
  // Suspend benchamrking for setup.
  folly::BenchmarkSuspender suspender;

  const RouterID vrfZero{0};

  SimPlatform plat(folly::MacAddress(), 128);
  std::vector<PortID> ports;
  for (int i = 0; i < 128; ++i) {
    ports.push_back(PortID(i));
  }
  cfg::SwitchConfig config =
      utility::onePortPerVlanConfig(plat.getHwSwitch(), ports);
  auto testHandle =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto sw = testHandle->getSw();

  // TODO(sas): This needs to go away after we are done transitionning to the
  // new RIB mechanism. RouteDistributionGenerator expects `RouteTables` to
  // have an entry for VRF 0 even if we are not using `RouteTables` at all
  // (i.e.: when the ENABLE_STANDALONE_RIB flag is set).
  sw->updateStateBlocking(
      "add VRF0", [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};
        auto newRouteTables = newState->getRouteTables()->modify(&newState);
        newRouteTables->addRouteTable(
            std::make_shared<RouteTable>(RouterID(0)));
        return newState;
      });

  auto routeChunks =
      Generator(sw->getAppliedState(), 1337, kEcmpWidth, vrfZero).get();

  // Resume benchmakring post-setup.
  suspender.dismiss();

  for (const auto& chunk : routeChunks) {
    std::vector<UnicastRoute> routesToAdd;
    std::vector<IpPrefix> routesToDel;

    for (const auto& route : chunk) {
      UnicastRoute routeToAdd;

      IpPrefix prefix;
      prefix.ip = facebook::network::toBinaryAddress(route.prefix.first);
      prefix.prefixLength = route.prefix.second;
      routeToAdd.dest_ref() = prefix;
      routeToAdd.nextHops_ref() = nextHopsThrift(route.nhops);

      routesToAdd.push_back(std::move(routeToAdd));
    }

    sw->getRib()->update(
        vrfZero,
        ClientID(10),
        AdminDistance::EBGP,
        routesToAdd,
        routesToDel,
        true /* sync */,
        "FibSync benchmark",
        [](RouterID vrf,
           const rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
           const rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
           void* cookie) {
          rib::ForwardingInformationBaseUpdater fibUpdater(
              vrf, v4NetworkToRoute, v6NetworkToRoute);
          static_cast<SwSwitch*>(cookie)->updateStateBlocking(
              "", std::move(fibUpdater));
        },
        sw);
  }
}

BENCHMARK(FibSyncFSWLegacy) {
  runOldRibTest<utility::FSWRouteScaleGenerator>();
}

BENCHMARK_RELATIVE(FibSyncFSW) {
  runNewRibTest<utility::FSWRouteScaleGenerator>();
}

BENCHMARK(FibSyncTHAlpmLegacy) {
  runOldRibTest<utility::THAlpmRouteScaleGenerator>();
}

BENCHMARK_RELATIVE(FibSyncTHAlpm) {
  runNewRibTest<utility::THAlpmRouteScaleGenerator>();
}

BENCHMARK(FibSyncHgridDuLegacy) {
  runOldRibTest<utility::HgridDuRouteScaleGenerator>();
}

BENCHMARK_RELATIVE(FibSyncHgridDu) {
  runNewRibTest<utility::HgridDuRouteScaleGenerator>();
}

BENCHMARK(FibSyncHgridUuLegacy) {
  runOldRibTest<utility::HgridUuRouteScaleGenerator>();
}

BENCHMARK_RELATIVE(FibSyncHgridUu) {
  runNewRibTest<utility::HgridUuRouteScaleGenerator>();
}

int main(int argc, char** argv) {
  facebook::initFacebook(&argc, &argv);
  folly::runBenchmarks();
  return EXIT_SUCCESS;
}
