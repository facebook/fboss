/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmMplsTestUtils.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/mpls.h>
#include <bcm/port.h>
}

namespace facebook::fboss {
class BcmIntfTable;
class BcmEcmpEgress;
class BcmMultiPathNextHop;
class BcmPortTable;
class BcmRouteTable;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

int getLabelSwappedWithForTopLabel(const HwSwitch* hwSwitch, uint32_t label) {
  auto unit =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  bcm_mpls_tunnel_switch_t info;
  bcm_mpls_tunnel_switch_t_init(&info);
  info.label = label;
  info.port = BCM_GPORT_INVALID;
  auto rv = bcm_mpls_tunnel_switch_get(unit, &info); // query label fib
  bcmCheckError(rv, "getLabelSwappedWithForTopLabel failed to query label fib");
  bcm_l3_egress_t egr;
  bcm_l3_egress_t_init(&egr);
  rv = bcm_l3_egress_get(0, info.egress_if, &egr); // query egress
  bcmCheckError(
      rv, "getLabelSwappedWithForTopLabel failed to query egress interface");
  return egr.mpls_label;
}

uint32_t getMaxLabelStackDepth(const HwSwitch* hwSwitch) {
  return static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
      ->getPlatform()
      ->maxLabelStackDepth();
}

template <typename AddrT>
void verifyLabeledNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    LabelForwardingEntry::Label label) {
  auto* bcmRoute = static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
                       ->routeTable()
                       ->getBcmRoute(0, prefix.network, prefix.mask);
  auto egressId = bcmRoute->getEgressId();
  utility::verifyLabeledEgress(egressId, label);
}
template void verifyLabeledNextHop<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV6>::Prefix prefix,
    LabelForwardingEntry::Label label);
template void verifyLabeledNextHop<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV4>::Prefix prefix,
    LabelForwardingEntry::Label label);

template <typename AddrT>
void verifyLabeledNextHopWithStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const LabelForwardingAction::LabelStack& stack) {
  auto* bcmRoute = static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
                       ->routeTable()
                       ->getBcmRoute(0, prefix.network, prefix.mask);
  auto egressId = bcmRoute->getEgressId();
  // verify that given egress is tunneled egress
  // its egress label must be tunnelLabel (top of stack)
  // rest of srack is from tunnel interface attached to egress
  utility::verifyTunneledEgress(egressId, stack);
}
template void verifyLabeledNextHopWithStack<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV6>::Prefix prefix,
    const LabelForwardingAction::LabelStack& stack);
template void verifyLabeledNextHopWithStack<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV4>::Prefix prefix,
    const LabelForwardingAction::LabelStack& stack);

template <typename AddrT>
void verifyMultiPathNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const std::map<PortDescriptor, LabelForwardingAction::LabelStack>& stacks,
    int numUnLabeledPorts,
    int numLabeledPorts) {
  auto* bcmRoute = static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
                       ->routeTable()
                       ->getBcmRoute(0, prefix.network, prefix.mask);
  auto egressId = bcmRoute->getEgressId(); // ecmp egress id

  std::map<bcm_port_t, LabelForwardingAction::LabelStack> bcmPort2Stacks;
  for (auto& entry : stacks) {
    auto portDesc = entry.first;
    auto& stack = entry.second;
    auto bcmPort = static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
                       ->getPortTable()
                       ->getBcmPortId(portDesc.phyPortID());
    bcmPort2Stacks.emplace(bcmPort, stack);
  }

  utility::verifyLabeledMultiPathEgress(
      numUnLabeledPorts, numLabeledPorts, egressId, bcmPort2Stacks);
}
template void verifyMultiPathNextHop<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV6>::Prefix prefix,
    const std::map<PortDescriptor, LabelForwardingAction::LabelStack>& stacks,
    int numUnLabeledPorts,
    int numLabeledPorts);
template void verifyMultiPathNextHop<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV4>::Prefix prefix,
    const std::map<PortDescriptor, LabelForwardingAction::LabelStack>& stacks,
    int numUnLabeledPorts,
    int numLabeledPorts);

template <typename AddrT>
void verifyLabeledMultiPathNextHopMemberWithStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    int memberIndex,
    const LabelForwardingAction::LabelStack& tunnelStack,
    bool resolved) {
  auto* bcmRoute = static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
                       ->routeTable()
                       ->getBcmRoute(0, prefix.network, prefix.mask);
  ASSERT_NE(bcmRoute, nullptr);
  const auto* egr =
      dynamic_cast<const BcmEcmpEgress*>(bcmRoute->getNextHop()->getEgress());
  ASSERT_NE(egr, nullptr);
  const auto& nextHops = egr->paths();
  EXPECT_GT(nextHops.size(), memberIndex);
  auto i = 0;
  for (const auto nextHopId : nextHops) {
    if (i == memberIndex) {
      // verify that given egress is tunneled egress
      // its egress label must be tunnelLabel (top of stack)
      // rest of srack is from tunnel interface attached to egress
      if (resolved) {
        utility::verifyTunneledEgress(nextHopId, tunnelStack);
      } else {
        utility::verifyTunneledEgressToDrop(nextHopId, tunnelStack);
      }
    }
    i++;
  }
}
template void verifyLabeledMultiPathNextHopMemberWithStack<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV6>::Prefix prefix,
    int memberIndex,
    const LabelForwardingAction::LabelStack& tunnelStack,
    bool resolved);
template void verifyLabeledMultiPathNextHopMemberWithStack<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV4>::Prefix prefix,
    int memberIndex,
    const LabelForwardingAction::LabelStack& tunnelStack,
    bool resolved);

long getTunnelRefCount(
    const HwSwitch* hwSwitch,
    InterfaceID intfID,
    const LabelForwardingAction::LabelStack& stack) {
  return static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
      ->getIntfTable()
      ->getBcmIntfIf(intfID)
      ->getLabeledTunnelRefCount(stack);
}
} // namespace facebook::fboss::utility
