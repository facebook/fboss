/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/AgentFeatures.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>

namespace facebook::fboss {

RouteNextHopSet makeNextHops(const std::vector<std::string>& ipsAsStrings) {
  RouteNextHopSet nhops;
  for (const std::string& ipAsString : ipsAsStrings) {
    nhops.emplace(UnresolvedNextHop(folly::IPAddress(ipAsString), ECMP_WEIGHT));
  }
  return nhops;
}

/*
 * Collect stats 10K times (100 time for VOQ) and benchmark that.
 * Using a fixed number rather than letting framework
 * pick a N for internal iteration, since
 * - We want a large enough number to notice any memory bloat
 *   in this code path. Relying on the framework to pick a large
 *   enough iteration for us is dicey
 * - Comparing 10K iterations of 2 versions of code seems sufficient
 *   for us. Having the framework be aware that we are doing internal
 *   iteration (by letting it pick number of iterations), and calculating
 *   cost of a single iterations does not seem to have more fidelity
 */
inline void runStatsCollectionBenchmark(bool alwaysCollectVoqStats = false) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<AgentEnsemble> ensemble{};
  // maximum 48 master logical ports (taken from wedge400) to get
  // consistent performance results across platforms with different
  // number of ports but same ASIC, e.g. wedge400 and minipack
  int numPortsToCollectStats = 48;
  // route counters in hardware is currently limited to 255.
  // this is due to the fact that in some platforms, route class id
  // (8 bits) is overloaded to support counter id.
  int numRouteCounters = 255;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [numPortsToCollectStats, numRouteCounters, alwaysCollectVoqStats](
          const AgentEnsemble& ensemble) {
        // Disable stats collection thread.
        FLAGS_enable_stats_update_thread = false;
        if (alwaysCollectVoqStats) {
          // Always collect VOQ stats for VOQ switches
          FLAGS_update_voq_stats_interval_s = 0;
        }
        // Disable DSF subscription on single-box test
        FLAGS_dsf_subscribe = false;
        // Enable fabric ports
        FLAGS_hide_fabric_ports = false;
        // Don't disable looped fabric ports
        FLAGS_disable_looped_fabric_ports = false;

        auto ports = ensemble.masterLogicalPortIds();
        auto portsNew = ports;

        bool hasFabric =
            ensemble.getSw()->getSwitchInfoTable().haveFabricSwitches();
        bool hasVoq = ensemble.getSw()->getSwitchInfoTable().haveVoqSwitches();

        if (!hasVoq && !hasFabric) {
          // Limit ports for non-VOQ and non-fabric switches
          portsNew.resize(
              std::min(static_cast<int>(ports.size()), numPortsToCollectStats));
        }

        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            portsNew,
            !hasFabric /* interfaceHasSubnet */,
            !hasFabric /* setInterfaceMac */,
            utility::kBaseVlanId,
            hasFabric || hasVoq /*enable fabric ports*/);
        if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
                HwAsic::Feature::ROUTE_COUNTERS)) {
          config.switchSettings()->maxRouteCounterIDs() = numRouteCounters;
        }
        if (ensemble.getSw()->getSwitchInfoTable().haveVoqSwitches()) {
          utility::addNetworkAIQosMaps(config, ensemble.getL3Asics());
          utility::setDefaultCpuTrafficPolicyConfig(
              config, ensemble.getL3Asics(), ensemble.isSai());
          utility::addCpuQueueConfig(
              config, ensemble.getL3Asics(), ensemble.isSai());
          config.dsfNodes() =
              *utility::addRemoteIntfNodeCfg(*config.dsfNodes());
          config.loadBalancers()->push_back(
              utility::getEcmpFullHashConfig(ensemble.getL3Asics()));
        }
        return config;
      };

  ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);
  int iterations = 10'000;

  // Setup Remote Intf and System Ports
  if (ensemble->getSw()->getSwitchInfoTable().haveVoqSwitches()) {
    utility::setupRemoteIntfAndSysPorts(
        ensemble->getSw(),
        ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    // For VOQ switches we have 2K - 4K remote system ports (each with 4-8
    // VOQs). This is >10x of local ports on NPU platforms. Therefore, only run
    // 100 iterations.
    iterations = 100;
  } else if (ensemble->getSw()->getSwitchInfoTable().haveL3Switches()) {
    if (checkSameAndGetAsic(ensemble->getSw()->getHwAsicTable()->getL3Asics())
            ->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      // TODO(Chenab): 10'000 iterations take 30 minutes on Chenab. Debug this
      // slowness
      iterations = 1000;
    }
  }

  std::vector<PortID> ports = ensemble->masterLogicalPortIds();
  ports.resize(
      std::min(static_cast<int>(ports.size()), numPortsToCollectStats));

  int maxRouteCounters = numRouteCounters;
  if (ensemble->getSw()->getSwitchInfoTable().haveL3Switches() &&
      checkSameAndGetAsic(ensemble->getSw()->getHwAsicTable()->getL3Asics())
              ->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
    // MT-762: counter Id 254 is preserved internally in SDK
    // >= 24.8.3001 for EBRO
    maxRouteCounters = 254;
  }

  if (ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::ROUTE_COUNTERS)) {
    auto updater = ensemble->getSw()->getRouteUpdater();
    for (auto i = 0; i < maxRouteCounters; i++) {
      folly::CIDRNetwork nw{
          folly::IPAddress(folly::sformat("2401:db00:0021:{:x}::", i)), 64};
      std::optional<RouteCounterID> counterID(std::to_string(i));
      UnicastRoute route = util::toUnicastRoute(
          nw,
          RouteNextHopEntry(
              makeNextHops({"1::"}), AdminDistance::EBGP, counterID));
      updater.addRoute(RouterID(0), ClientID::BGPD, route);
    }
    updater.program();
  }

  suspender.dismiss();
  for (auto i = 0; i < iterations; ++i) {
    ensemble->getSw()->updateStats();
  }
  suspender.rehire();
}

} // namespace facebook::fboss
