// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/agent/state/LabelForwardingEntry.h>
#include <array>

namespace facebook::fboss {

namespace util {

LabelForwardingAction getSwapAction(LabelForwardingAction::Label swapWith);

LabelForwardingAction getPhpAction(bool isPhp = true);

LabelForwardingAction getPushAction(LabelForwardingAction::LabelStack stack);

LabelNextHopEntry getSwapLabelNextHopEntry(
    AdminDistance distance,
    InterfaceID intfId = InterfaceID(1),
    std::vector<folly::IPAddress> addrs = {});

LabelNextHopEntry getPushLabelNextHopEntry(
    AdminDistance distance,
    InterfaceID intfId = InterfaceID(1),
    std::vector<folly::IPAddress> addrs = {});

LabelNextHopEntry getPhpLabelNextHopEntry(
    AdminDistance distance,
    InterfaceID intfId = InterfaceID(1),
    std::vector<folly::IPAddress> addrs = {});

LabelNextHopEntry getPopLabelNextHopEntry(
    AdminDistance distance,
    InterfaceID intfId = InterfaceID(1),
    std::vector<folly::IPAddress> addrs = {});

std::vector<MplsRoute> getTestRoutes(int base = 0, int count = 4);

} // namespace util

} // namespace facebook::fboss
