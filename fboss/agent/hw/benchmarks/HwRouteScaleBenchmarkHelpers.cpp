/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

namespace facebook::fboss {

std::unique_ptr<AgentEnsemble> setupForVoqRouteScale(uint32_t ecmpWidth) {
  // Allow 100% ECMP resource usage
  FLAGS_ecmp_resource_percentage = 100;
  FLAGS_ecmp_width = ecmpWidth;

  AgentEnsembleSwitchConfigFn voqInitialConfig =
      [](const AgentEnsemble& ensemble) {
        FLAGS_hide_fabric_ports = false;
        FLAGS_dsf_subscribe = false;
        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            ensemble.masterLogicalPortIds(),
            true, /*interfaceHasSubnet*/
            true, /*setInterfaceMac*/
            utility::kBaseVlanId,
            true /*enable fabric ports*/);
        utility::populatePortExpectedNeighborsToSelf(
            ensemble.masterLogicalPortIds(), config);
        config.dsfNodes() = *utility::addRemoteIntfNodeCfg(*config.dsfNodes());
        if (FLAGS_enable_ecmp_resource_manager) {
          config.switchSettings()->ecmpCompressionThresholdPct() = 100;
        }
        return config;
      };
  auto ensemble =
      createAgentEnsemble(voqInitialConfig, false /*disableLinkStateToggler*/);
  auto updateDsfStateFn = [&ensemble](const std::shared_ptr<SwitchState>& in) {
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
    utility::populateRemoteIntfAndSysPorts(
        switchId2SystemPorts,
        switchId2Rifs,
        ensemble->getSw()->getConfig(),
        ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    return DsfStateUpdaterUtil::getUpdatedState(
        in,
        ensemble->getSw()->getScopeResolver(),
        ensemble->getSw()->getRib(),
        switchId2SystemPorts,
        switchId2Rifs);
  };
  ensemble->getSw()->getRib()->updateStateInRibThread(
      [&ensemble, updateDsfStateFn]() {
        ensemble->getSw()->updateStateWithHwFailureProtection(
            folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
      });
  return ensemble;
}

std::vector<RoutePrefixV6> getVoqRoutePrefixes(uint32_t numRoutes) {
  std::vector<RoutePrefixV6> prefixes;
  for (int i = 0; i < numRoutes; i++) {
    prefixes.emplace_back(
        folly::IPAddressV6(folly::sformat("2401:db00:23{}::", i + 1)), 48);
  }
  return prefixes;
}

std::vector<flat_set<PortDescriptor>> getVoqRouteNextHopSets(
    const boost::container::flat_set<PortDescriptor>& remoteNhops,
    uint32_t ecmpGroup,
    uint32_t ecmpWidth) {
  std::vector<flat_set<PortDescriptor>> nhopSets;
  CHECK_GE(remoteNhops.size(), ecmpWidth + ecmpGroup - 1);
  auto kNhopOffset = 4;
  for (int i = 0; i < ecmpGroup; i++) {
    auto sysPortStart = (i * kNhopOffset);
    nhopSets.emplace_back(
        std::make_move_iterator(remoteNhops.begin() + sysPortStart),
        std::make_move_iterator(
            remoteNhops.begin() + sysPortStart + ecmpWidth));
  }
  return nhopSets;
}

void voqRouteBenchmark(bool add, uint32_t ecmpGroup, uint32_t ecmpWidth) {
  folly::BenchmarkSuspender suspender;
  auto ensemble = setupForVoqRouteScale(ecmpWidth);
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto remoteNhops = utility::resolveRemoteNhops(ensemble.get(), ecmpHelper);
  auto nhopSets = getVoqRouteNextHopSets(remoteNhops, ecmpGroup, ecmpWidth);
  auto prefixes = getVoqRoutePrefixes(ecmpGroup);
  auto programRoutes = [&]() {
    auto updater = ensemble->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&updater, nhopSets, prefixes);
  };
  auto unprogramRoutes = [&]() {
    auto updater = ensemble->getSw()->getRouteUpdater();
    ecmpHelper.unprogramRoutes(&updater, prefixes);
  };
  if (add) {
    suspender.dismiss();
    programRoutes();
    suspender.rehire();
    unprogramRoutes();
  } else {
    programRoutes();
    suspender.dismiss();
    unprogramRoutes();
    suspender.rehire();
  }
}
} // namespace facebook::fboss
