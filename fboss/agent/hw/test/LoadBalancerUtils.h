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
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"
#include "folly/MacAddress.h"

#include <optional>

#include <vector>

namespace facebook::fboss {
class HwSwitch;
class Platform;
class SwitchState;
class LoadBalancer;
class HwSwitchEnsemble;
class SwitchIdScopeResolver;

} // namespace facebook::fboss

namespace facebook::fboss::utility {

inline const std::string kUdfGroupName("dstQueuePair");
inline const std::string kUdfPktMatcherName("l4UdpRoce");
inline const int kUdfStartOffsetInBytes(13);
inline const int kUdfFieldSizeInBytes(3);
inline const int kUdfL4DstPort(4791);
inline const int kRandomUdfL4SrcPort(62946);

cfg::LoadBalancer getEcmpHalfHashConfig(const Platform* platform);
cfg::LoadBalancer getEcmpFullHashConfig(const Platform* platform);
cfg::LoadBalancer getEcmpFullUdfHashConfig(const Platform* platform);
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig(
    const Platform* platform);
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig(
    const Platform* platform);
std::vector<cfg::LoadBalancer> getEcmpFullTrunkFullHashConfig(
    const Platform* platform);

std::shared_ptr<SwitchState> setLoadBalancer(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const cfg::LoadBalancer& loadBalancer,
    const SwitchIdScopeResolver& resolver);

std::shared_ptr<SwitchState> addLoadBalancers(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const std::vector<cfg::LoadBalancer>& loadBalancers,
    const SwitchIdScopeResolver& resolver);

void pumpTraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255,
    std::optional<folly::MacAddress> srcMac = std::nullopt);

void pumpRoCETraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int roceDestPort = kUdfL4DstPort, /* RoCE fixed dst port */
    int hopLimit = 255,
    std::optional<folly::MacAddress> srcMacAddr = std::nullopt);

void pumpTraffic(
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::vector<folly::IPAddress> srcIp,
    std::vector<folly::IPAddress> dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t streams,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255,
    std::optional<folly::MacAddress> srcMac = std::nullopt,
    int numPkts = 1000);

void pumpDeterministicRandomTraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress intfMac,
    VlanID vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255);

void pumpMplsTraffic(
    bool isV6,
    HwSwitch* hw,
    uint32_t label,
    folly::MacAddress intfMac,
    VlanID vlanId,
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

bool isLoadBalanced(
    const std::map<PortID, HwPortStats>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false);

bool isLoadBalanced(
    const std::map<PortID, HwPortStats>& portStats,
    int maxDeviationPct);

bool isLoadBalanced(
    const std::map<std::string, HwPortStats>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    /* Flag to control whether having no traffic on a link is ok */
    bool noTrafficOk = false);

bool isLoadBalanced(
    const std::map<std::string, HwPortStats>& portStats,
    int maxDeviationPct);

bool isLoadBalanced(
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getPortStatsFn,
    int maxDeviationPct,
    bool noTrafficOk = false);

bool pumpTrafficAndVerifyLoadBalanced(
    std::function<void()> pumpTraffic,
    std::function<void()> clearPortStats,
    std::function<bool()> isLoadBalanced,
    int retries = 3);

cfg::UdfConfig addUdfConfig();

} // namespace facebook::fboss::utility
