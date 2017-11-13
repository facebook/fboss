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
  using RoutesV4Delta = NodeMapDelta<NodeMapRibV4>;
  using RoutesV6Delta = NodeMapDelta<NodeMapRibV6>;

  using DeltaValue<RouteTable>::DeltaValue;

  RoutesV4Delta getRoutesV4Delta() const {
    return RoutesV4Delta(
      getOld() ? getOld()->getRibV4()->routes().get() : nullptr,
      getNew() ? getNew()->getRibV4()->routes().get() : nullptr);
  }
  RoutesV6Delta getRoutesV6Delta()  const {
    return RoutesV6Delta(
      getOld() ? getOld()->getRibV6()->routes().get() : nullptr,
      getNew() ? getNew()->getRibV6()->routes().get() : nullptr);
  }
};

typedef NodeMapDelta<RouteTableMap, RouteTablesDelta> RTMapDelta;

}}
