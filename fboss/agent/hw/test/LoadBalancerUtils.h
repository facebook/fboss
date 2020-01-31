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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"

#include <optional>

#include <vector>

namespace facebook::fboss {
class HwSwitch;
class Platform;
class SwitchState;
class LoadBalancer;
class HwSwitchEnsemble;

} // namespace facebook::fboss

namespace facebook::fboss::utility {

cfg::LoadBalancer getEcmpHalfHashConfig(const Platform* platform);
cfg::LoadBalancer getEcmpFullHashConfig(const Platform* platform);
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig(
    const Platform* platform);
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig(
    const Platform* platform);

std::shared_ptr<SwitchState> addLoadBalancer(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const cfg::LoadBalancer& loadBalancer);

std::shared_ptr<SwitchState> addLoadBalancers(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const std::vector<cfg::LoadBalancer>& loadBalancers);

void pumpTraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress intfMac,
    VlanID vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt);

void pumpMplsTraffic(
    bool isV6,
    HwSwitch* hw,
    uint32_t label,
    folly::MacAddress intfMac,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt);

bool isLoadBalanced(
    HwSwitchEnsemble* hwSwitchEnsemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false);

bool isLoadBalanced(
    HwSwitchEnsemble* hwSwitchEnsemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    int maxDeviationPct);

} // namespace facebook::fboss::utility
