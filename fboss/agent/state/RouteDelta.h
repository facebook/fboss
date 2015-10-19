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

#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"


namespace facebook { namespace fboss {

class RouteTablesDelta : public DeltaValue<RouteTable> {
 public:
  using NodeMapRibV4 = RouteTableRibNodeMap<folly::IPAddressV4>;
  using NodeMapRibV6 = RouteTableRibNodeMap<folly::IPAddressV6>;
  using RoutesV4Delta = NodeMapDelta<NodeMapRibV4,
        DeltaValue<NodeMapRibV4::Node>,
        MapUniquePointerTraits<NodeMapRibV4>>;
  using RoutesV6Delta = NodeMapDelta<NodeMapRibV6,
        DeltaValue<NodeMapRibV6::Node>,
        MapUniquePointerTraits<NodeMapRibV6>>;

  using DeltaValue<RouteTable>::DeltaValue;

  RoutesV4Delta getRoutesV4Delta() const {
    std::unique_ptr<NodeMapRibV4> oldRib, newRib;
    if (getOld()) {
      oldRib.reset(new NodeMapRibV4());
      oldRib->addRoutes(*(getOld()->getRibV4()));
    }
    if (getNew()) {
      newRib.reset(new NodeMapRibV4());
      newRib->addRoutes(*(getNew()->getRibV4()));
    }
    return RoutesV4Delta(std::move(oldRib), std::move(newRib));
  }
  RoutesV6Delta getRoutesV6Delta()  const {
    std::unique_ptr<NodeMapRibV6> oldRib, newRib;
    if (getOld()) {
      oldRib.reset(new NodeMapRibV6());
      oldRib->addRoutes(*(getOld()->getRibV6()));
    }
    if (getNew()) {
      newRib.reset(new NodeMapRibV6());
      newRib->addRoutes(*(getNew()->getRibV6()));
    }
    return RoutesV6Delta(std::move(oldRib), std::move(newRib));
  }
};

typedef NodeMapDelta<RouteTableMap, RouteTablesDelta> RTMapDelta;

}}
