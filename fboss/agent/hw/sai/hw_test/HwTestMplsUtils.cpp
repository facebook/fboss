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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/gen/Base.h>

namespace facebook::fboss::utility {

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
void verifyLabeledNextHop(
    const HwSwitch* /* unused */,
    typename Route<AddrT>::Prefix /* unused */,
    LabelForwardingEntry::Label /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
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
    const HwSwitch* /* unused */,
    typename Route<AddrT>::Prefix /* unused */,
    const LabelForwardingAction::LabelStack& /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
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
    const HwSwitch* /* unused */,
    typename Route<AddrT>::Prefix /* unused */,
    const std::
        map<PortDescriptor, LabelForwardingAction::LabelStack>& /* unused */,
    int /* unused */,
    int /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
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

// Verifies that the stack is programmed correctly on the interface
void verifyProgrammedStackOnInterface(
    const HwSwitch* /* unused */,
    const InterfaceID& /* unused */,
    const LabelForwardingAction::LabelStack& /* unused */,
    long /* unused */) {
  throw FbossError("Unimplemented Test Case for SAI");
}

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
