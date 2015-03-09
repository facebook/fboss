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
  typedef NodeMapDelta<RouteTableRib<folly::IPAddressV4>> RoutesV4Delta;
  typedef NodeMapDelta<RouteTableRib<folly::IPAddressV6>> RoutesV6Delta;
  using DeltaValue<RouteTable>::DeltaValue;
  RoutesV4Delta getRoutesV4Delta() const {
    return RoutesV4Delta(getOld() ? getOld()->getRibV4().get() : nullptr,
                         getNew() ? getNew()->getRibV4().get() : nullptr);
  }
  RoutesV6Delta getRoutesV6Delta() const {
    return RoutesV6Delta(getOld() ? getOld()->getRibV6().get() : nullptr,
                         getNew() ? getNew()->getRibV6().get() : nullptr);
  }
};

typedef NodeMapDelta<RouteTableMap, RouteTablesDelta> RTMapDelta;

}}
