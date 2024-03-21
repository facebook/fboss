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

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/types.h"
#include "folly/MacAddress.h"

#include <folly/gen/Base.h>
#include <optional>
#include <vector>

namespace facebook::fboss {
class HwSwitch;
class Platform;
class SwitchState;
class LoadBalancer;
class HwSwitchEnsemble;
class SwitchIdScopeResolver;
class SwSwitch;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

template <typename PortIdT, typename PortStatsT>
bool isLoadBalanced(
    TestEnsembleIf* ensemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false) {
  auto getPortStatsFn = [&](const std::vector<PortIdT>& portIds)
      -> std::map<PortIdT, PortStatsT> {
    return ensemble->getLatestPortStats(portIds);
  };
  return isLoadBalanced<PortIdT, PortStatsT>(
      ecmpPorts, weights, getPortStatsFn, maxDeviationPct, noTrafficOk);
}

template <typename PortIdT, typename PortStatsT>
bool isLoadBalanced(
    HwSwitchEnsemble* hwSwitchEnsemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    int maxDeviationPct) {
  return isLoadBalanced<PortIdT, PortStatsT>(
      hwSwitchEnsemble,
      ecmpPorts,
      std::vector<NextHopWeight>(),
      maxDeviationPct);
}

void pumpTrafficAndVerifyLoadBalanced(
    std::function<void()> pumpTraffic,
    std::function<void()> clearPortStats,
    std::function<bool()> isLoadBalanced,
    bool loadBalanceExpected = true);

bool isHwDeterministicSeed(
    HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    LoadBalancerID id);

} // namespace facebook::fboss::utility
