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
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <boost/noncopyable.hpp>
#include <cstdint>

namespace facebook::fboss {
class HwSwitch;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

int getLabelSwappedWithForTopLabel(const HwSwitch* hwSwitch, uint32_t label);
uint32_t getMaxLabelStackDepth(const HwSwitch* hwSwitch);

template <typename AddrT>
void verifyLabeledNextHop(
    const HwSwitch* hwSwitch,
    typename Route<AddrT>::Prefix prefix,
    LabelForwardingEntry::Label label);

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

// Verifies that the stack is programmed correctly on the interface
void verifyProgrammedStackOnInterface(
    const HwSwitch* hwSwitch,
    const InterfaceID& intfID,
    const LabelForwardingAction::LabelStack& stack,
    long refCount);

long getTunnelRefCount(
    const HwSwitch* hwSwitch,
    InterfaceID intfID,
    const LabelForwardingAction::LabelStack& stack);
} // namespace facebook::fboss::utility
