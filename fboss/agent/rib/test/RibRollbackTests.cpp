/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/Utils.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

using facebook::network::toBinaryAddress;
using folly::IPAddress;

constexpr AdminDistance kBgpDistance = AdminDistance::EBGP;
constexpr ClientID kBgpClient = ClientID::BGPD;
const RouterID kRid(0);
auto kPrefix1 = IPAddress::createNetwork("1::1/64");
auto kPrefix2 = IPAddress::createNetwork("2::2/64");

void recordUpdates(
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto nextStatePtr =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);

  // Update routes in switch state captured in cookie
  *nextStatePtr = fibUpdater(*nextStatePtr);
  (*nextStatePtr)->publish();
}

class FailSomeUpdates {
 public:
  explicit FailSomeUpdates(std::unordered_set<int> toFail)
      : toFail_(std::move(toFail)) {}
  void operator()(
      RouterID /*vrf*/,
      const IPv4NetworkToRouteMap& /*v4NetworkToRoute*/,
      const IPv6NetworkToRouteMap& /*v6NetworkToRoute*/,
      void* /*cookie*/) {
    if (toFail_.find(++cnt_) != toFail_.end()) {
      throw FbossHwUpdateError(nullptr, nullptr);
    }
  }

 private:
  int cnt_{0};
  std::unordered_set<int> toFail_;
};

IpPrefix makePrefix(const folly::CIDRNetwork& nw) {
  IpPrefix pfx;
  pfx.ip_ref() = toBinaryAddress(nw.first);
  pfx.prefixLength_ref() = nw.second;
  return pfx;
}

UnicastRoute makeUnicastRoute(
    const folly::CIDRNetwork& nw,
    RouteForwardAction action,
    AdminDistance distance = kBgpDistance) {
  UnicastRoute nr;
  nr.dest_ref() = makePrefix(nw);
  nr.action_ref() = action;
  nr.adminDistance_ref() = distance;
  return nr;
}

TEST(RibRollbackTest, rollbackFail) {
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  FailSomeUpdates failUpdateAndRollback({1, 2});
  EXPECT_DEATH(
      rib.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
          {},
          false,
          "add only",
          failUpdateAndRollback,
          nullptr),
      ".*");
}

TEST(RibRollbackTest, rollbackAdd) {
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();
  auto origSwitchState = switchState;
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  rib.update(
      kRid,
      kBgpClient,
      kBgpDistance,
      {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
      {},
      false,
      "add only",
      recordUpdates,
      &switchState);
  EXPECT_NE(switchState, origSwitchState);
  EXPECT_EQ(1, switchState->getGeneration());
  auto assertRouteCount = [&]() {
    auto [numV4, numV6] = switchState->getFibs()->getRouteCount();
    EXPECT_EQ(1, numV6);
    EXPECT_EQ(0, numV4);
    EXPECT_EQ(1, rib.getRouteTableDetails(kRid).size());
  };
  assertRouteCount();
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix2, RouteForwardAction::DROP)},
          {},
          false,
          "fail add",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount();
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib.getRouteTableDetails(kRid));
}

TEST(RibRollbackTest, rollbackDel) {
  auto switchState = std::make_shared<SwitchState>();
  switchState->publish();
  auto origSwitchState = switchState;
  RoutingInformationBase rib;
  rib.ensureVrf(kRid);
  rib.update(
      kRid,
      kBgpClient,
      kBgpDistance,
      {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
      {},
      false,
      "add only",
      recordUpdates,
      &switchState);
  EXPECT_NE(switchState, origSwitchState);
  EXPECT_EQ(1, switchState->getGeneration());
  auto assertRouteCount = [&]() {
    auto [numV4, numV6] = switchState->getFibs()->getRouteCount();
    EXPECT_EQ(1, numV6);
    EXPECT_EQ(0, numV4);
    EXPECT_EQ(1, rib.getRouteTableDetails(kRid).size());
  };
  assertRouteCount();
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {},
          {makePrefix(kPrefix1)},
          false,
          "fail del",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount();
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib.getRouteTableDetails(kRid));
}
