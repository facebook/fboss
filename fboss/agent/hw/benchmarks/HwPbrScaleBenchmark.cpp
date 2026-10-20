/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <algorithm>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

DEFINE_int32(
    pbr_policy_scale,
    100,
    "Number of PBR class-based policies to program before collecting stats");

namespace facebook::fboss {

namespace {

constexpr int kClassesPerPolicy = 4;
constexpr int kMembersPerNhg = 2;
constexpr int kMaxNexthops = 64;
constexpr int kStatsCollectionIterations = 100;

std::string makeNhgName(int index) {
  return fmt::format("pbrNhg{:05d}", index);
}

std::string makePolicyName(int index) {
  return fmt::format("pbrPolicy{:05d}", index);
}

std::vector<RouteNextHopSet> makeDistinctNhgSets(
    const utility::EcmpSetupAnyNPorts6& ecmpHelper,
    int numNhops,
    int count) {
  std::vector<RouteNextHopSet> sets;
  sets.reserve(count);
  auto done = [&]() { return static_cast<int>(sets.size()) == count; };
  for (int first = 0; first < numNhops && !done(); ++first) {
    for (int second = first + 1; second < numNhops && !done(); ++second) {
      std::vector<ResolvedNextHop> members;
      members.reserve(kMembersPerNhg);
      for (auto index : {first, second}) {
        const auto& nhop = ecmpHelper.nhop(index);
        members.emplace_back(folly::IPAddress(nhop.ip), nhop.intf, ECMP_WEIGHT);
      }
      sets.emplace_back(members.begin(), members.end());
    }
  }
  CHECK_EQ(static_cast<int>(sets.size()), count)
      << "only " << numNhops << " next hops: cannot build " << count
      << " distinct " << kMembersPerNhg << "-member groups";
  return sets;
}

folly::IPAddressV6 makeRoutePrefix(int index) {
  return folly::IPAddressV6(
      fmt::format("2222:db8:{:x}:{:x}::", (index >> 8) + 1, index & 0xFF));
}

// A policy only synthesizes ACL entries once a route references it, so the
// benchmark has to bind one route per policy to have anything to collect.
void bindRoutesToPolicies(AgentEnsemble& ensemble, int numPolicies) {
  auto updater = ensemble.getSw()->getRouteUpdater();
  for (int i = 0; i < numPolicies; ++i) {
    UnicastRoute route;
    route.dest()->ip() = facebook::network::toBinaryAddress(
        folly::IPAddress(makeRoutePrefix(i)));
    route.dest()->prefixLength() = 64;
    NamedRouteDestination namedDest;
    namedDest.policyName() = makePolicyName(i);
    route.namedRouteDestination() = namedDest;
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
  }
  updater.program();
}

std::vector<ClassBasedPolicy> makePbrPolicy(int numPolicies) {
  std::vector<ClassBasedPolicy> policies;
  policies.reserve(numPolicies);
  for (int i = 0; i < numPolicies; ++i) {
    ClassBasedPolicy policy;
    policy.name() = makePolicyName(i);
    policy.defaultNexthopGroup() = makeNhgName(i);
    NextHopGroup redirectGroup;
    redirectGroup.name() = makeNhgName((i + 1) % numPolicies);
    for (int tc = 1; tc <= kClassesPerPolicy; ++tc) {
      policy.class2NextHopGroup()[static_cast<ForwardingClass>(tc)] =
          redirectGroup;
    }
    policies.push_back(std::move(policy));
  }
  return policies;
}

size_t programmedPbrEntries(const AgentEnsemble& ensemble) {
  auto acls = ensemble.getSw()->getState()->getAclsForTable(
      cfg::AclStage::INGRESS,
      cfg::switch_config_constants::DEFAULT_PBR_ACL_TABLE());
  return acls ? acls->size() : 0;
}

AgentEnsembleSwitchConfigFn makePbrConfigFn() {
  return [](const AgentEnsemble& ensemble) {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addAclTable(
        &cfg,
        cfg::switch_config_constants::DEFAULT_PBR_ACL_TABLE(),
        1 /*priority*/,
        {cfg::AclTableActionType::COUNTER, cfg::AclTableActionType::REDIRECT},
        {cfg::AclTableQualifier::TC,
         cfg::AclTableQualifier::NEXT_HOP_GROUP_ID});
    return cfg;
  };
}

void pbrAclStatsCollectionBenchmark(int numPolicies) {
  folly::BenchmarkSuspender suspender;

  FLAGS_enable_nexthop_id_manager = true;
  FLAGS_enable_stats_update_thread = false;

  auto ensemble =
      createAgentEnsemble(makePbrConfigFn(), false /*disableLinkStateToggler*/);
  ensemble->stopStatsThread();

  if (!ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::PBR_ACL)) {
    return;
  }

  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto numNhops =
      std::min(static_cast<int>(ecmpHelper.getNextHops().size()), kMaxNexthops);
  CHECK_GT(numNhops, 0);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, numNhops);
  });

  if (numPolicies > 0) {
    auto nhgSets = makeDistinctNhgSets(ecmpHelper, numNhops, numPolicies);
    std::vector<std::pair<std::string, RouteNextHopSet>> groups;
    groups.reserve(numPolicies);
    for (int i = 0; i < numPolicies; ++i) {
      groups.emplace_back(makeNhgName(i), std::move(nhgSets[i]));
    }
    ensemble->getSw()->getRib()->addOrUpdateNamedNextHopGroups(
        ensemble->getSw()->getScopeResolver(),
        groups,
        createRibToSwitchStateFunction(),
        ensemble->getSw());

    ensemble->getSw()->getRib()->addOrUpdatePolicies(
        makePbrPolicy(numPolicies),
        createNextHopIdStateUpdate(ensemble->getSw(), "addOrUpdatePolicies"));

    bindRoutesToPolicies(*ensemble, numPolicies);
  }
  CHECK_EQ(
      programmedPbrEntries(*ensemble),
      static_cast<size_t>(kClassesPerPolicy * numPolicies));
  suspender.dismiss();
  for (int i = 0; i < kStatsCollectionIterations; ++i) {
    ensemble->getSw()->updateStats();
  }
  suspender.rehire();
}

} // namespace

BENCHMARK(HwPbrAclStatsCollectionBenchmark) {
  pbrAclStatsCollectionBenchmark(FLAGS_pbr_policy_scale);
}

} // namespace facebook::fboss
