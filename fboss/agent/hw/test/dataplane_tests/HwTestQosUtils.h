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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

#include <chrono>
#include <map>
#include <vector>

namespace facebook::fboss {
class HwSwitchEnsemble;
struct HwPortStats;
class HwSwitch;
class PortDescriptor;
class SwitchState;

namespace utility {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort);

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort);

void verifyVoQHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::SystemPortID egressPort);

template <typename SendPktFunT>
void sendPktAndVerifyQueueHit(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    HwSwitchEnsemble* ensemble,
    const SendPktFunT& sendPacket,
    PortID portId,
    std::optional<SystemPortID> sysPortId) {
  for (const auto& q2dscps : q2dscpMap) {
    for (auto dscp : q2dscps.second) {
      auto portStatsBefore = ensemble->getLatestPortStats(portId);
      HwSysPortStats sysPortStatsBefore;
      if (sysPortId) {
        sysPortStatsBefore = ensemble->getLatestSysPortStats(*sysPortId);
      }
      sendPacket(dscp);
      verifyQueueHit(portStatsBefore, q2dscps.first, ensemble, portId);
      if (sysPortId) {
        verifyVoQHit(sysPortStatsBefore, q2dscps.first, ensemble, *sysPortId);
      }
    }
  }
}

bool verifyQueueMappingsInvariantHelper(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const std::vector<PortID>& ecmpPorts,
    uint32_t sleep = 20);

void disableTTLDecrements(
    HwSwitch* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop);

void disableTTLDecrements(HwSwitch* /*hw*/, const PortDescriptor& /*port*/);

template <typename EcmpNhopT>
void disableTTLDecrements(
    HwSwitch* hw,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  auto asic = hw->getPlatform()->getAsic();
  if (asic->isSupported(HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    disableTTLDecrements(hw, routerId, nhop.intf, folly::IPAddress(nhop.ip));
  } else if (asic->isSupported(HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    disableTTLDecrements(hw, nhop.portDesc);
  } else {
    throw FbossError("Disable decrement not supported");
  }
}

template <typename EcmpNhopT>
void ttlDecrementHandlingForLoopbackTraffic(
    HwSwitch* hw,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  auto asic = hw->getPlatform()->getAsic();
  // for TTL0 supported devices we need to go through cfg change
  if (!asic->isSupported(HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    disableTTLDecrements(hw, routerId, nhop);
  }
}

} // namespace utility
} // namespace facebook::fboss
