/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#include "RouteTable.h"

#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/FbossError.h"

namespace {
constexpr auto kRouterId = "routerId";
constexpr auto kRibV4 = "ribV4";
constexpr auto kRibV6 = "ribV6";
}

namespace facebook { namespace fboss {

RouteTableFields::RouteTableFields(RouterID id)
    : id(id),
      ribV4(std::make_shared<RibTypeV4>()),
      ribV6(std::make_shared<RibTypeV6>()) {
}

folly::dynamic RouteTableFields::toFollyDynamic() const {
  folly::dynamic rtable = folly::dynamic::object;
  rtable[kRouterId] = static_cast<uint32_t>(id);
  rtable[kRibV4] = ribV4->toFollyDynamic();
  rtable[kRibV6] = ribV6->toFollyDynamic();
  return rtable;
}

RouteTableFields
RouteTableFields::fromFollyDynamic(const folly::dynamic& rtableJson) {
  RouteTableFields rtable(RouterID(rtableJson[kRouterId].asInt()));
  rtable.ribV4 = RibTypeV4::fromFollyDynamic(rtableJson[kRibV4]);
  rtable.ribV6 = RibTypeV6::fromFollyDynamic(rtableJson[kRibV6]);
  return rtable;
}

RouteTable::RouteTable(RouterID id) : NodeBaseT(id) {
}

RouteTable::~RouteTable() {
}

bool RouteTable::empty() const {
  return getRibV4()->empty() && getRibV6()->empty();
}

template class NodeBaseT<RouteTable, RouteTableFields>;

}}
