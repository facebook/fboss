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

#include <folly/Optional.h>

#include <vector>

namespace facebook {
namespace fboss {
class HwSwitch;
class Platform;
class SwitchState;
class LoadBalancer;
class HwSwitchEnsemble;
} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

cfg::Fields getHalfHashFields();
cfg::Fields getFullHashFields();
cfg::LoadBalancer getHalfHashConfig(cfg::LoadBalancerID id);
cfg::LoadBalancer getFullHashConfig(cfg::LoadBalancerID id);
cfg::LoadBalancer getEcmpHalfHashConfig();
cfg::LoadBalancer getEcmpFullHashConfig();
cfg::LoadBalancer getTrunkHalfHashConfig();
cfg::LoadBalancer getTrunkFullHashConfig();
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig();
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig();

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
    folly::MacAddress cpuMac,
    folly::Optional<PortID> frontPanelPortToLoopTraffic = folly::none);

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
} // namespace utility
} // namespace fboss
} // namespace facebook
