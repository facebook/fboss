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

DECLARE_string(load_balance_traffic_src);

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

struct SendPktFunc {
  using FuncType3 = std::function<void(
      std::unique_ptr<TxPacket>,
      std::optional<PortDescriptor>,
      std::optional<uint8_t>)>;

  explicit SendPktFunc(FuncType3 func) : func3_(std::move(func)) {}

  void operator()(
      std::unique_ptr<TxPacket> pkt,
      std::optional<PortDescriptor> port = std::nullopt,
      std::optional<uint8_t> queue = std::nullopt);

 private:
  const FuncType3 func3_;
};
SendPktFunc getSendPktFunc(TestEnsembleIf* ensemble);
SendPktFunc getSendPktFunc(SwSwitch* sw);

using AllocatePktFunc = std::function<std::unique_ptr<TxPacket>(uint32_t size)>;
AllocatePktFunc getAllocatePktFn(TestEnsembleIf* ensemble);
AllocatePktFunc getAllocatePktFn(SwSwitch* sw);

size_t pumpTraffic(
    bool isV6,
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255,
    int numPackets = 10000,
    std::optional<folly::MacAddress> srcMac = std::nullopt);

size_t pumpRoCETraffic(
    bool isV6,
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int roceDestPort = kUdfL4DstPort, /* RoCE fixed dst port */
    int hopLimit = 255,
    std::optional<folly::MacAddress> srcMacAddr = std::nullopt,
    int packetCount = 50000);

size_t pumpTrafficWithSourceFile(
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255,
    std::optional<folly::MacAddress> srcMacAddr = std::nullopt);

void pumpTraffic(
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
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
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress intfMac,
    VlanID vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt,
    int hopLimit = 255);

void pumpMplsTraffic(
    bool isV6,
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    uint32_t label,
    folly::MacAddress intfMac,
    VlanID vlanId,
    std::optional<PortID> frontPanelPortToLoopTraffic = std::nullopt);

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
