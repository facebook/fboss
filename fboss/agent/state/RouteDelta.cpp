/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteDelta.h"

#include "Route.h"

#include "fboss/agent/state/NodeMapDelta-defs.h"

namespace facebook { namespace fboss {

template class NodeMapDelta<RouteTableMap, RouteTablesDelta>;

using NodeMapRibV4 = RouteTableRibNodeMap<folly::IPAddressV4>;
template class NodeMapDelta<NodeMapRibV4>;

using NodeMapRibV6 = RouteTableRibNodeMap<folly::IPAddressV6>;
template class NodeMapDelta<NodeMapRibV6>;
}}
