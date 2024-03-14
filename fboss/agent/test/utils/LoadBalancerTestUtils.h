// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestEnsembleIf.h"

#include <folly/gen/Base.h>

namespace facebook::fboss::utility {

template <typename PortIdT, typename PortStatsT>
bool isLoadBalancedImpl(
    const std::map<PortIdT, PortStatsT>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template <typename PortStatsT>
bool isLoadBalanced(
    const std::map<std::string, PortStatsT>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false) {
  return isLoadBalancedImpl(portStats, weights, maxDeviationPct, noTrafficOk);
}

template <typename PortIdT, typename PortStatsT>
bool isLoadBalanced(
    const std::map<PortIdT, PortStatsT>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false) {
  return isLoadBalancedImpl(portStats, weights, maxDeviationPct, noTrafficOk);
}

template <typename PortIdT, typename PortStatsT>
bool isLoadBalanced(
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    const std::function<std::map<PortIdT, PortStatsT>(
        const std::vector<PortIdT>&)>& getPortStatsFn,
    int maxDeviationPct,
    bool noTrafficOk = false) {
  auto portIDs =
      folly::gen::from(ecmpPorts) | folly::gen::map([](const auto& portDesc) {
        if constexpr (std::is_same_v<PortStatsT, HwPortStats>) {
          return portDesc.phyPortID();
        } else if constexpr (std::is_same_v<PortStatsT, HwSysPortStats>) {
          return portDesc.sysPortID();
        }
        throw FbossError("Unsupported port stats type in isLoadBalanced");
      }) |
      folly::gen::as<std::vector<PortIdT>>();
  auto portIdToStats = getPortStatsFn(portIDs);
  return isLoadBalancedImpl(
      portIdToStats, weights, maxDeviationPct, noTrafficOk);
}

template <typename PortIdT, typename PortStatsT>
bool isLoadBalanced(
    const std::map<PortIdT, PortStatsT>& portStats,
    int maxDeviationPct) {
  return isLoadBalanced(
      portStats, std::vector<NextHopWeight>(), maxDeviationPct);
}

template <typename PortStatsT>
bool isLoadBalanced(
    const std::map<std::string, PortStatsT>& portStats,
    int maxDeviationPct) {
  return isLoadBalanced(
      portStats, std::vector<NextHopWeight>(), maxDeviationPct);
}

inline const int kScalingFactor(100);
inline const int kLoadWeight(70);
inline const int kQueueWeight(30);

cfg::UdfConfig addUdfHashConfig();
cfg::UdfConfig addUdfAclConfig();
cfg::UdfConfig addUdfHashAclConfig();

cfg::FlowletSwitchingConfig getDefaultFlowletSwitchingConfig();
void addFlowletAcl(cfg::SwitchConfig& cfg);
void addFlowletConfigs(
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports);

cfg::LoadBalancer getTrunkHalfHashConfig(const HwAsic& asic);
cfg::LoadBalancer getEcmpHalfHashConfig(const HwAsic& asic);
cfg::LoadBalancer getEcmpFullHashConfig(const HwAsic& asic);
cfg::LoadBalancer getEcmpFullUdfHashConfig(const HwAsic& asic);
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig(
    const HwAsic& asic);
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig(
    const HwAsic& asic);
std::vector<cfg::LoadBalancer> getEcmpFullTrunkFullHashConfig(
    const HwAsic& asic);

std::shared_ptr<SwitchState> setLoadBalancer(
    TestEnsembleIf* ensemble,
    const std::shared_ptr<SwitchState>& inputState,
    const cfg::LoadBalancer& loadBalancer,
    const SwitchIdScopeResolver& resolver);

std::shared_ptr<SwitchState> addLoadBalancers(
    TestEnsembleIf* ensemble,
    const std::shared_ptr<SwitchState>& inputState,
    const std::vector<cfg::LoadBalancer>& loadBalancers,
    const SwitchIdScopeResolver& resolver);
} // namespace facebook::fboss::utility
