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
#include <fboss/agent/hw/test/HwSwitchEnsemble.h>
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <boost/noncopyable.hpp>
#include <cstdint>

namespace facebook::fboss {
class HwSwitch;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

int getLabelSwappedWithForTopLabel(const HwSwitch* hwSwitch, uint32_t label);

// Utility functions used for HwLabelEdgeRouteTest
template <typename AddrT>
void verifyLabeledNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    Label label);

template <typename AddrT>
void verifyLabeledNextHopWithStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const LabelForwardingAction::LabelStack& stack);

template <typename AddrT>
void verifyMultiPathNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const std::map<PortDescriptor, LabelForwardingAction::LabelStack>& stacks,
    int numUnLabeledPorts,
    int numLabeledPorts);

template <typename AddrT>
void verifyLabeledMultiPathNextHopMemberWithStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    int memberIndex,
    const LabelForwardingAction::LabelStack& tunnelStack,
    bool resolved);

// Verifies that the stack is programmed correctly
template <typename AddrT>
void verifyProgrammedStack(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    const InterfaceID& intfID,
    const LabelForwardingAction::LabelStack& stack,
    long refCount);

// Utility functions used for HwLabelSwitchRouteTest
template <typename AddrT>
void verifyLabelSwitchAction(
    const HwSwitch* hwSwitch,
    Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const EcmpMplsNextHop<AddrT>& nexthop);

template <typename AddrT>
void verifyMultiPathLabelSwitchAction(
    const HwSwitch* hwSwitch,
    Label label,
    const LabelForwardingAction::LabelForwardingType action,
    const std::vector<EcmpMplsNextHop<AddrT>>& nexthops);

uint64_t getMplsDestNoMatchCounter(
    HwSwitchEnsemble* ensemble,
    const std::shared_ptr<SwitchState> state,
    PortID inPort);
} // namespace facebook::fboss::utility
