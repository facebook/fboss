/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;
class RouteManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
    setupPorts();
    setupVlans();
    setupInterfaces();
    setupNeighbors();
  }

  void setupPorts() {
    addPort(1, true);
    addPort(2, true);
  }
  void setupVlans() {
    addVlan(1, {});
    addVlan(2, {});
  }
  void setupInterfaces() {
    addInterface(1, rmac1);
    addInterface(2, rmac2);
  }
  void setupNeighbors() {
    addArpEntry(1, dip1.asV4(), dmac1);
    addArpEntry(2, dip2.asV4(), dmac2);
  }

  folly::IPAddress dip1{"1.1.1.1"};
  folly::IPAddress dip2{"2.2.2.2"};
  folly::MacAddress rmac1{"41:41:41:41:41:41"};
  folly::MacAddress rmac2{"42:42:42:42:42:42"};
  folly::MacAddress dmac1{"11:11:11:11:11:11"};
  folly::MacAddress dmac2{"22:22:22:22:22:22"};

};

TEST_F(RouteManagerTest, addRoute) {
  RouteFields<folly::IPAddressV4>::Prefix dest;
  dest.network = folly::IPAddressV4{"42.42.42.42"};
  dest.mask = 24;
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(dest);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(RouterID(0), r);
}
