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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

namespace {

constexpr int kPbrPolicyScale = 100;
constexpr int kMaxNexthops = 64;
constexpr int kStatsCollectionIterations = 100;

std::string makeNhgName(int index) {
  return fmt::format("pbrNhg{:05d}", index);
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
    std::vector<std::pair<std::string, RouteNextHopSet>> groups;
    groups.reserve(numPolicies);
    for (int i = 0; i < numPolicies; ++i) {
      const auto& first = ecmpHelper.nhop(i % numNhops);
      const auto& second = ecmpHelper.nhop((i + 1) % numNhops);
      groups.emplace_back(
          makeNhgName(i),
          RouteNextHopSet{
              ResolvedNextHop(
                  folly::IPAddress(first.ip), first.intf, ECMP_WEIGHT),
              ResolvedNextHop(
                  folly::IPAddress(second.ip), second.intf, ECMP_WEIGHT)});
    }
    ensemble->getSw()->getRib()->addOrUpdateNamedNextHopGroups(
        ensemble->getSw()->getScopeResolver(),
        groups,
        createRibToSwitchStateFunction(),
        ensemble->getSw());
  }
  suspender.dismiss();
  for (int i = 0; i < kStatsCollectionIterations; ++i) {
    ensemble->getSw()->updateStats();
  }
  suspender.rehire();
}

} // namespace

BENCHMARK(HwPbrAclStatsCollectionBenchmark) {
  pbrAclStatsCollectionBenchmark(kPbrPolicyScale);
}

} // namespace facebook::fboss
