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
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"

#include <folly/gen/Base.h>

namespace facebook::fboss::utility {
const auto kRouter0 = RouterID(0);
/*
 * Retrieve the next label swapped with the given the label entry
 */
int getLabelSwappedWithForTopLabel(const HwSwitch* hwSwitch, uint32_t label) {
  auto switchId = static_cast<const facebook::fboss::SaiSwitch*>(hwSwitch)
                      ->managerTable()
                      ->switchManager()
                      .getSwitchSaiId();
  auto& mplsApi = SaiApiTable::getInstance()->mplsApi();
  SaiInSegTraits::InSegEntry is{switchId, label};
  auto nextHopId =
      mplsApi.getAttribute(is, SaiInSegTraits::Attributes::NextHopId());
  auto& nextHopApi = SaiApiTable::getInstance()->nextHopApi();
  auto labelStack = nextHopApi.getAttribute(
      NextHopSaiId(nextHopId), SaiMplsNextHopTraits::Attributes::LabelStack{});
  if (labelStack.size() != 1) {
    throw FbossError(
        "getLabelSwappedWithForTopLabel to retrieve label stack to swap with.");
  }
  return labelStack.back();
}

template <typename AddrT>
sai_object_id_t getNextHopId(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix) {
  auto saiSwitch = static_cast<const facebook::fboss::SaiSwitch*>(hwSwitch);
  // Query the nexthop ID given the route prefix
  folly::IPAddress prefixNetwork{prefix.network};
  folly::CIDRNetwork follyPrefix{prefixNetwork, prefix.mask};
  auto virtualRouterHandle =
      saiSwitch->managerTable()->virtualRouterManager().getVirtualRouterHandle(
          kRouter0);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router with id 0");
  }
  auto routeEntry = SaiRouteTraits::RouteEntry(
      saiSwitch->getSwitchId(),
      virtualRouterHandle->virtualRouter->adapterKey(),
      follyPrefix);
  return SaiApiTable::getInstance()->routeApi().getAttribute(
      routeEntry, SaiRouteTraits::Attributes::NextHopId());
}

template <typename AddrT>
NextHopSaiId getNextHopSaiId(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix) {
  return static_cast<NextHopSaiId>(getNextHopId<AddrT>(hwSwitch, prefix));
}

template <typename AddrT>
NextHopGroupSaiId getNextHopGroupSaiId(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix) {
  return static_cast<NextHopGroupSaiId>(getNextHopId<AddrT>(hwSwitch, prefix));
}

NextHopSaiId getNextHopSaiIdForMember(NextHopGroupMemberSaiId member) {
  return static_cast<NextHopSaiId>(
      SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
          member, SaiNextHopGroupMemberTraits::Attributes::NextHopId{}));
}

std::vector<NextHopSaiId> getNextHopMembers(NextHopGroupSaiId group) {
  auto members = SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
      group, SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
  std::vector<NextHopSaiId> nexthops{};
  for (auto member : members) {
    auto nexthop =
        getNextHopSaiIdForMember(static_cast<NextHopGroupMemberSaiId>(member));
    nexthops.push_back(nexthop);
  }
  return nexthops;
}

template <typename AddrT>
void verifyLabeledNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    LabelForwardingEntry::Label label) {
  auto& nextHopApi = SaiApiTable::getInstance()->nextHopApi();
  auto labelStack = nextHopApi.getAttribute(
      getNextHopSaiId<AddrT>(hwSwitch, prefix),
      SaiMplsNextHopTraits::Attributes::LabelStack{});
  EXPECT_GT(labelStack.size(), 0);
  EXPECT_EQ(labelStack[0], label);
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
  auto nextHopId = getNextHopSaiId<AddrT>(hwSwitch, prefix);
  auto& nextHopApi = SaiApiTable::getInstance()->nextHopApi();
  auto labelStack = nextHopApi.getAttribute(
      NextHopSaiId(nextHopId), SaiMplsNextHopTraits::Attributes::LabelStack{});
  EXPECT_EQ(labelStack.size(), stack.size());
  for (auto i = 0; i < stack.size(); i++) {
    EXPECT_EQ(labelStack[i], stack[i]);
  }
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
  auto group = getNextHopGroupSaiId<AddrT>(hwSwitch, prefix);
  auto nexthops = getNextHopMembers(group);
  EXPECT_EQ((numUnLabeledPorts + numLabeledPorts), nexthops.size());

  std::vector<LabelForwardingAction::LabelStack> nexthopStacks;
  for (auto nexthop : nexthops) {
    auto stack = SaiApiTable::getInstance()->nextHopApi().getAttribute(
        nexthop, SaiMplsNextHopTraits::Attributes::LabelStack{});
    LabelForwardingAction::LabelStack labelStack;
    for (auto label : stack) {
      labelStack.push_back(label);
    }
    nexthopStacks.push_back(std::move(labelStack));
  }
  EXPECT_EQ(numLabeledPorts, nexthopStacks.size());
  for (auto entry : stacks) {
    auto stack = entry.second;
    nexthopStacks.erase(std::remove_if(
        nexthopStacks.begin(),
        nexthopStacks.end(),
        [stack](const auto& nexthopStack) { return nexthopStack == stack; }));
  }
  EXPECT_EQ(nexthopStacks.size(), 0);
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
    const HwSwitch* /* unused */,
    typename Route<AddrT>::Prefix /* unused */,
    int /* unused */,
    const LabelForwardingAction::LabelStack& /* unused */,
    bool /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
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

// Verifies that the stack is programmed correctly.
// Reference count has no meaning in SAI and we assume that there always is a
// labelStack programmed for nexthop when this function is called.
template <typename AddrT>
void verifyProgrammedStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const InterfaceID& /* unused */,
    const LabelForwardingAction::LabelStack& stack,
    long /* unused */) {
  auto& nextHopApi = SaiApiTable::getInstance()->nextHopApi();
  auto nextHopId = getNextHopSaiId<AddrT>(hwSwitch, prefix);
  auto labelStack = nextHopApi.getAttribute(
      nextHopId, SaiMplsNextHopTraits::Attributes::LabelStack{});

  // If stack is empty, simply check if the labelStack is programmed
  // If the stack is empty, check if the labelstack has a label with MPLS
  // Else, check that the stacks match
  if (stack.empty()) {
    EXPECT_EQ(labelStack.size(), 1);
    EXPECT_EQ(
        nextHopApi.getAttribute(
            nextHopId, SaiMplsNextHopTraits::Attributes::Type()),
        SAI_NEXT_HOP_TYPE_MPLS);
  } else {
    EXPECT_EQ(labelStack.size(), stack.size());
    for (int i = 0; i < labelStack.size(); i++) {
      EXPECT_EQ(labelStack[i], stack[i]);
    }
  }
}
template void verifyProgrammedStack<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV6>::Prefix prefix,
    const InterfaceID& intfID,
    const LabelForwardingAction::LabelStack& stack,
    long refCount);
template void verifyProgrammedStack<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    typename Route<folly::IPAddressV4>::Prefix prefix,
    const InterfaceID& intfID,
    const LabelForwardingAction::LabelStack& stack,
    long refCount);

template <typename AddrT>
void verifyLabelSwitchAction(
    const HwSwitch* /* unused */,
    const LabelForwardingEntry::Label /* unused */,
    const LabelForwardingAction::LabelForwardingType /* unused */,
    const EcmpMplsNextHop<AddrT>& /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
}
template void verifyLabelSwitchAction<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    const LabelForwardingEntry::Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const EcmpMplsNextHop<folly::IPAddressV6>& nexthop);
template void verifyLabelSwitchAction<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    const LabelForwardingEntry::Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const EcmpMplsNextHop<folly::IPAddressV4>& nexthop);

template <typename AddrT>
void verifyMultiPathLabelSwitchAction(
    const HwSwitch* /* unused */,
    const LabelForwardingEntry::Label /* unused */,
    const LabelForwardingAction::LabelForwardingType /* unused */,
    const std::vector<EcmpMplsNextHop<AddrT>>& /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
}
template void verifyMultiPathLabelSwitchAction<folly::IPAddressV6>(
    const HwSwitch* hwSwitch,
    const LabelForwardingEntry::Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const std::vector<EcmpMplsNextHop<folly::IPAddressV6>>& nexthops);
template void verifyMultiPathLabelSwitchAction<folly::IPAddressV4>(
    const HwSwitch* hwSwitch,
    const LabelForwardingEntry::Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const std::vector<EcmpMplsNextHop<folly::IPAddressV4>>& nexthops);
} // namespace facebook::fboss::utility
