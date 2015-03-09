/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/RouteTableMap.h"

#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook { namespace fboss {

RouteTableMap::RouteTableMap() {
}

RouteTableMap::~RouteTableMap() {
}

void RouteTableMap::getRouteCount(uint64_t *v4Count, uint64_t *v6Count) {
  uint64_t v4 = 0;
  uint64_t v6 = 0;
  for (const auto& table : getAllNodes()) {
    v4 += table.second->getRibV4()->size();
    v6 += table.second->getRibV6()->size();
  }
  *v4Count = v4;
  *v6Count = v6;
}

FBOSS_INSTANTIATE_NODE_MAP(RouteTableMap, RouteTableMapTraits);

}} // facebook::fboss
