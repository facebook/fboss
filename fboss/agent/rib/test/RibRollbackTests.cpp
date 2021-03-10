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
constexpr AdminDistance kOpenrDistance = AdminDistance::OPENR;
constexpr ClientID kOpenrClient = ClientID::OPENR;
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

class RibRollbackTest : public ::testing::Test {
 public:
  void SetUp() override {
    rib_.ensureVrf(kRid);
    switchState_ = std::make_shared<SwitchState>();
    auto origSwitchState = switchState_;
    switchState_->publish();
    rib_.update(
        kRid,
        kBgpClient,
        kBgpDistance,
        {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
        {},
        false,
        "add only",
        recordUpdates,
        &switchState_);
    EXPECT_NE(switchState_, origSwitchState);
    EXPECT_EQ(1, switchState_->getGeneration());
    assertRouteCount(0, 1);
  }
  void assertRouteCount(int v4Expected, int v6Expected) const {
    auto [numV4, numV6] = switchState_->getFibs()->getRouteCount();
    EXPECT_EQ(v6Expected, numV6);
    EXPECT_EQ(v4Expected, numV4);
    EXPECT_EQ(v6Expected + v4Expected, rib_.getRouteTableDetails(kRid).size());
  }

 protected:
  RoutingInformationBase rib_;
  std::shared_ptr<SwitchState> switchState_;
};

TEST_F(RibRollbackTest, rollbackFail) {
  FailSomeUpdates failUpdateAndRollback({1, 2});
  EXPECT_DEATH(
      rib_.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix2, RouteForwardAction::DROP)},
          {},
          false,
          "add only",
          failUpdateAndRollback,
          nullptr),
      ".*");
}

TEST_F(RibRollbackTest, rollbackAdd) {
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
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
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackAddExisting) {
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
          {},
          false,
          "fail add",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackDel) {
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
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
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackDelNonExistent) {
  auto routeTableBeforeUpdate = rib_.getRouteTableDetails(kRid);
  // Noop update - prefix does not exist in rib
  rib_.update(
      kRid,
      kBgpClient,
      kBgpDistance,
      {},
      {makePrefix(kPrefix2)}, // kPrefix2 does not exist in RIB
      false,
      "nonexistent prefix del",
      recordUpdates,
      &switchState_);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeUpdate, rib_.getRouteTableDetails(kRid));
  // Failed update, still rib is left as is
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {},
          {makePrefix(kPrefix2)}, // kPrefix2 does not exist in RIB
          false,
          "fail del",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackAddAndDel) {
  // Fail route update. Rib should rollback to pre failed add state
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix2, RouteForwardAction::DROP)},
          {makePrefix(kPrefix1)},
          false,
          "fail add",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackDifferentClient) {
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
          kRid,
          kOpenrClient,
          kOpenrDistance,
          {makeUnicastRoute(kPrefix1, RouteForwardAction::DROP)},
          {},
          false,
          "fail add",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}

TEST_F(RibRollbackTest, rollbackDifferentNexthops) {
  // Test rollback for route replacement
  auto routeTableBeforeFailedUpdate = rib_.getRouteTableDetails(kRid);
  FailSomeUpdates failFirstUpdate({1});
  EXPECT_THROW(
      rib_.update(
          kRid,
          kBgpClient,
          kBgpDistance,
          {makeUnicastRoute(kPrefix1, RouteForwardAction::TO_CPU)},
          {},
          false,
          "fail add",
          failFirstUpdate,
          nullptr),
      FbossHwUpdateError);
  assertRouteCount(0, 1);
  EXPECT_EQ(routeTableBeforeFailedUpdate, rib_.getRouteTableDetails(kRid));
}
