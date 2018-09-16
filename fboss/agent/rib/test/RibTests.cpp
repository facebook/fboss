/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RouteNextHop.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteTypes.h"

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <folly/IPAddress.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(RoutePrefix, Initialization) {
  RoutePrefixV4 v4Prefix;
  RoutePrefixV6 v6Prefix;
}

TEST(NextHop, Initialization) {
  folly::IPAddress addr("255.0.0.255");
  NextHopWeight weight(80);

  UnresolvedNextHop(addr, weight);
  ResolvedNextHop(addr, InterfaceID(1), weight);
}

TEST(NextHopEntry, Initialization) {
  RouteNextHopEntry entry(
      RouteForwardAction::TO_CPU, AdminDistance::STATIC_ROUTE);
}
