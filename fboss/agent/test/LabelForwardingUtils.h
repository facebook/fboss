// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/agent/state/LabelForwardingEntry.h>
#include <array>

namespace facebook {
namespace fboss {
namespace util {

LabelForwardingAction getSwapAction(LabelForwardingAction::Label swapWith);

LabelForwardingAction getPhpAction(bool isPhp = true);

LabelForwardingAction getPushAction(LabelForwardingAction::LabelStack stack);

LabelNextHopEntry getSwapLabelNextHopEntry(AdminDistance distance);

LabelNextHopEntry getPushLabelNextHopEntry(AdminDistance distance);

LabelNextHopEntry getPhpLabelNextHopEntry(AdminDistance distance);

LabelNextHopEntry getPopLabelNextHopEntry(AdminDistance distance);

} // namespace util
} // namespace fboss
} // namespace facebook
