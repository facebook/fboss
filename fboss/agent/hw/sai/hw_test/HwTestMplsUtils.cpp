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
  // TODO
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
  // TODO
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
  // TODO
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
  // TODO
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
    const HwSwitch* /* unused */,
    InterfaceID /* unused */,
    const LabelForwardingAction::LabelStack& /* unused */) {
  // Tunnel is not yet implemented in SAI.
  return 0;
}
} // namespace facebook::fboss::utility
