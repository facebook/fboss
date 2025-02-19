// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>

#include <gtest/gtest.h>
#include <chrono>
#include <map>
#include <vector>

namespace facebook::fboss::utility {

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop);

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& port);

template <typename EcmpNhopT>
std::shared_ptr<SwitchState> disableTTLDecrements(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  if (asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    return disableTTLDecrement(
        asicTable, state, routerId, nhop.intf, folly::IPAddress(nhop.ip));
  } else if (asicTable->isFeatureSupportedOnAnyAsic(
                 HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    return disableTTLDecrement(asicTable, state, nhop.portDesc);
  } else {
    throw FbossError("Disable decrement not supported");
  }
}

template <typename EcmpNhopT>
void disableTTLDecrements(
    SwSwitch* sw,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  sw->updateStateBlocking(
      "disable TTL decrement",
      [sw, &routerId, &nhop](const std::shared_ptr<SwitchState>& state) {
        auto newState = utility::disableTTLDecrements(
            sw->getHwAsicTable(), state, routerId, nhop);
        return newState;
      });
}

template <typename EcmpNhopT>
void disableTTLDecrements(
    SwSwitch* sw,
    RouterID routerId,
    const std::vector<EcmpNhopT>& nhops) {
  sw->updateStateBlocking(
      "disable TTL decrement",
      [sw, &routerId, &nhops](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        for (const auto& nhop : nhops) {
          newState = utility::disableTTLDecrements(
              sw->getHwAsicTable(), newState, routerId, nhop);
        }
        return newState;
      });
}

void disableTTLDecrements(
    TestEnsembleIf* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop);

void disableTTLDecrements(TestEnsembleIf* hw, const PortDescriptor& port);

template <typename EcmpNhopT>
void disableTTLDecrements(
    TestEnsembleIf* ensemble,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  if (ensemble->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    disableTTLDecrements(
        ensemble, routerId, nhop.intf, folly::IPAddress(nhop.ip));
  } else if (ensemble->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
                 HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    disableTTLDecrements(ensemble, nhop.portDesc);
  } else {
    throw FbossError("Disable decrement not supported");
  }
}

void disableTTLDecrementOnPorts(
    SwSwitch* sw,
    const boost::container::flat_set<PortDescriptor>& ecmpPorts);

/*
 * Disable TTL decrement on a set of ports
 */
void disableTTLDecrements(
    SwSwitch* sw,
    const boost::container::flat_set<PortDescriptor>& ecmpPorts);

template <typename EcmpNhopT>
void ttlDecrementHandlingForLoopbackTraffic(
    TestEnsembleIf* hw,
    RouterID routerId,
    const EcmpNhopT& nhop) {
  auto asicTable = hw->getHwAsicTable();
  // for TTL0 supported devices we need to go through cfg change
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    disableTTLDecrements(hw, routerId, nhop);
  }
}

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::PortID egressPort,
    int delta = 1);

void verifyVoQHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::SystemPortID egressPort,
    int delta = 1);

template <typename SendPktFunT>
void sendPktAndVerifyQueueHit(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    SwSwitch* sw,
    const SendPktFunT& sendPacket,
    PortID portId,
    std::optional<SystemPortID> sysPortId) {
  for (const auto& q2dscps : q2dscpMap) {
    WITH_RETRIES({
      auto portsStats = sw->getHwPortStats({portId});
      EXPECT_EVENTUALLY_NE(portsStats.find(portId), portsStats.end());
      auto portStatsBefore = portsStats[portId];
      HwSysPortStats sysPortStatsBefore;
      if (sysPortId) {
        auto sysportsStats = sw->getHwSysPortStats({*sysPortId});
        EXPECT_EVENTUALLY_NE(
            sysportsStats.find(*sysPortId), sysportsStats.end());
        sysPortStatsBefore = sysportsStats[*sysPortId];
      }
      for (auto dscp : q2dscps.second) {
        XLOG(DBG2) << "send packet with dscp " << int(dscp) << " to queue "
                   << q2dscps.first;
        sendPacket(dscp);
      }
      int delta = q2dscps.second.size();
      verifyQueueHit(portStatsBefore, q2dscps.first, sw, portId, delta);
      if (sysPortId) {
        verifyVoQHit(sysPortStatsBefore, q2dscps.first, sw, *sysPortId, delta);
      }
    });
  }
}

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const PortID portId,
    uint32_t sleep = 200);

template <typename SwitchT>
bool verifyQueueMappingsInvariantHelper(
    const std::map<int, std::vector<uint8_t>>& q2dscpMap,
    SwitchT* sw,
    std::shared_ptr<SwitchState> swState,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const std::vector<PortID>& ecmpPorts,
    uint32_t sleep = 20) {
  auto portStatsBefore = getAllHwPortStats();
  auto vlanId = utility::firstVlanID(swState);
  auto intfMac = utility::getFirstInterfaceMac(swState);
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

  for (const auto& q2dscps : q2dscpMap) {
    for (auto dscp : q2dscps.second) {
      auto pkt = makeTCPTxPacket(
          sw,
          vlanId,
          srcMac,
          intfMac,
          folly::IPAddressV6("1::10"),
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
          8000,
          8001,
          dscp << 2);
      if constexpr (std::is_same_v<SwitchT, HwSwitch>) {
        sw->sendPacketSwitchedSync(std::move(pkt));
      } else {
        sw->sendPacketSwitchedAsync(std::move(pkt));
      }
    }
  }

  bool mappingVerified = false;
  for (auto& ecmpPort : ecmpPorts) {
    // Since we don't know which port the above IP will get hashed to,
    // iterate over all ports in ecmp group to find one which satisfies
    // dscp to queue mapping.
    if (mappingVerified) {
      XLOG(DBG2) << "Mapping verified!";
      break;
    }

    XLOG(DBG2) << "Verifying mapping for ecmp port: " << (int)ecmpPort;
    mappingVerified = verifyQueueMappings(
        portStatsBefore[ecmpPort],
        q2dscpMap,
        getAllHwPortStats,
        ecmpPort,
        sleep);
  }
  return mappingVerified;
}

template <typename EnsembleT>
bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    EnsembleT* ensemble,
    facebook::fboss::PortID egressPort) {
  // lambda that returns HwPortStats for the given port
  auto getPortStats = [ensemble,
                       egressPort]() -> std::map<PortID, HwPortStats> {
    std::vector<facebook::fboss::PortID> portIds = {egressPort};
    return ensemble->getLatestPortStats(portIds);
  };

  return verifyQueueMappings(
      portStatsBefore, q2dscps, getPortStats, egressPort);
}

int getMaxWeightWRRQueue(const std::map<int, uint8_t>& queueToWeight);

} // namespace facebook::fboss::utility
