// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_constants.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteNextHopsMulti.h"

namespace facebook::fboss {

using LabelNextHop = ResolvedNextHop;
using LabelNextHopEntry = RouteNextHopEntry;
using LabelNextHopSet = RouteNextHopSet;
using LabelNextHopsByClient = RouteNextHopsMulti;

using LabelForwardingEntry = Route<LabelID>;
} // namespace facebook::fboss
