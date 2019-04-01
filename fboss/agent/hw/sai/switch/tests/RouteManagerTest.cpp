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
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE |
        SetupStage::NEIGHBOR;
    ManagerTestBase::SetUp();
  }

  ResolvedNextHop makeSwNextHop(int id) {
    const auto& testInterface = testInterfaces.at(id);
    const auto& remote = testInterface.remoteHosts.at(0);
    return ResolvedNextHop{remote.ip, InterfaceID(id), ECMP_WEIGHT};
  }

};

TEST_F(RouteManagerTest, addRoute) {
  RouteFields<folly::IPAddressV4>::Prefix dest;
  dest.network = folly::IPAddressV4{"42.42.42.42"};
  dest.mask = 24;
  ResolvedNextHop nh1 = makeSwNextHop(1);
  ResolvedNextHop nh2 = makeSwNextHop(2);
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(dest);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(RouterID(0), r);
}
