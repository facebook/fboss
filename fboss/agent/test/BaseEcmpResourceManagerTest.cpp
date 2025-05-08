/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

RouteNextHopSet makeNextHops(int n) {
  CHECK_LT(n, 255);
  RouteNextHopSet h;
  const InterfaceID kRandomInterfaceId{1};
  for (int i = 0; i < n; i++) {
    std::stringstream ss;
    ss << std::hex << i + 1;
    auto ipStr = "100::" + ss.str();
    h.emplace(ResolvedNextHop(
        folly::IPAddress(ipStr), kRandomInterfaceId, UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

RouteV6::Prefix makePrefix(int offset) {
  std::stringstream ss;
  ss << std::hex << offset;
  return RouteV6::Prefix(
      folly::IPAddressV6(folly::sformat("2601:db00:2110:{}::", ss.str())), 64);
}

std::shared_ptr<RouteV6> makeRoute(
    const RouteV6::Prefix& pfx,
    const RouteNextHopSet& nextHops) {
  RouteNextHopEntry nhopEntry(nextHops, kDefaultAdminDistance);
  auto rt = std::make_shared<RouteV6>(
      RouteV6::makeThrift(pfx, ClientID(0), nhopEntry));
  rt->setResolved(nhopEntry);
  rt->publish();
  return rt;
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::consolidate(
    const std::shared_ptr<SwitchState>& state) {
  state->publish();
  StateDelta delta(state_, state);
  auto deltas = consolidator_->consolidate(delta);
  consolidator_->updateDone(delta);
  if (deltas.size()) {
    XLOG(DBG2) << " Checking deltas, num deltas: " << deltas.size();
    assertDeltasForOverflow(deltas);
    state_ = deltas.back().newState();
  } else {
    state_ = state;
  }
  state_->publish();
  EXPECT_EQ(state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(), 0);
  return deltas;
}

void BaseEcmpResourceManagerTest::assertDeltasForOverflow(
    const std::vector<StateDelta>& deltas) const {
  std::map<RouteNextHopSet, uint32_t> primaryEcmpTypeGroups2RefCnt;
  for (const auto& [_, route] :
       std::as_const(*cfib(deltas.begin()->oldState()))) {
    if (route->isResolved() &&
        !route->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      auto pitr = primaryEcmpTypeGroups2RefCnt.find(
          route->getForwardInfo().normalizedNextHops());
      if (pitr != primaryEcmpTypeGroups2RefCnt.end()) {
        ++pitr->second;
        XLOG(DBG4) << "Processed route: " << route->str()
                   << " primary ECMP groups count unchanged: "
                   << primaryEcmpTypeGroups2RefCnt.size();
      } else {
        primaryEcmpTypeGroups2RefCnt.insert(
            {route->getForwardInfo().normalizedNextHops(), 1});
        XLOG(DBG4) << "Processed route: " << route->str()
                   << " primary ECMP groups count incremented: "
                   << primaryEcmpTypeGroups2RefCnt.size();
      }
    }
  }
  XLOG(DBG2) << " Primary ECMP groups : "
             << primaryEcmpTypeGroups2RefCnt.size();
  // ECMP groups should not exceed the limit.
  EXPECT_LE(
      primaryEcmpTypeGroups2RefCnt.size(),
      consolidator_->getMaxPrimaryEcmpGroups());
  auto routeDeleted = [&primaryEcmpTypeGroups2RefCnt](const auto& oldRoute) {
    XLOG(DBG2) << " Route deleted: " << oldRoute->str();
    if (oldRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      return;
    }
    auto pitr = primaryEcmpTypeGroups2RefCnt.find(
        oldRoute->getForwardInfo().normalizedNextHops());
    ASSERT_NE(pitr, primaryEcmpTypeGroups2RefCnt.end());
    EXPECT_GE(pitr->second, 1);
    --pitr->second;
    if (pitr->second == 0) {
      primaryEcmpTypeGroups2RefCnt.erase(pitr);
      XLOG(DBG2) << " Primary ECMP group count decremented to: "
                 << primaryEcmpTypeGroups2RefCnt.size()
                 << " on pfx: " << oldRoute->str();
    }
  };
  auto routeAdded = [&primaryEcmpTypeGroups2RefCnt,
                     this](const auto& newRoute) {
    XLOG(DBG2) << " Route added: " << newRoute->str();
    if (newRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      return;
    }
    auto pitr = primaryEcmpTypeGroups2RefCnt.find(
        newRoute->getForwardInfo().normalizedNextHops());
    if (pitr != primaryEcmpTypeGroups2RefCnt.end()) {
      ++pitr->second;
    } else {
      bool inserted{false};
      std::tie(pitr, inserted) = primaryEcmpTypeGroups2RefCnt.insert(
          {newRoute->getForwardInfo().normalizedNextHops(), 1});
      EXPECT_TRUE(inserted);
      XLOG(DBG2) << " Primary ECMP group count incremented to: "
                 << primaryEcmpTypeGroups2RefCnt.size()
                 << " on pfx: " << newRoute->str();
    }
    EXPECT_LE(
        primaryEcmpTypeGroups2RefCnt.size(),
        consolidator_->getMaxPrimaryEcmpGroups());
  };

  for (const auto& delta : deltas) {
    XLOG(DBG2) << " Processing delta";
    forEachChangedRoute<folly::IPAddressV6>(
        delta,
        [=](RouterID /*rid*/, const auto& oldRoute, const auto& newRoute) {
          if (!oldRoute->isResolved() && !newRoute->isResolved()) {
            return;
          }
          if (oldRoute->isResolved() && !newRoute->isResolved()) {
            routeDeleted(oldRoute);
            return;
          }
          if (!oldRoute->isResolved() && newRoute->isResolved()) {
            routeAdded(newRoute);
            return;
          }
          // Both old and new are resolved
          CHECK(oldRoute->isResolved() && newRoute->isResolved());
          if (oldRoute->getForwardInfo() != newRoute->getForwardInfo()) {
            // First process as a add route, since ECMP is make before break
            routeAdded(newRoute);
            routeDeleted(oldRoute);
          }
        },
        [=](RouterID /*rid*/, const auto& newRoute) {
          if (newRoute->isResolved()) {
            routeAdded(newRoute);
          }
        },
        [=](RouterID /*rid*/, const auto& oldRoute) {
          if (oldRoute->isResolved()) {
            routeDeleted(oldRoute);
          }
        });
  }
}
RouteV6::Prefix BaseEcmpResourceManagerTest::nextPrefix() const {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto offset = 0; offset < std::numeric_limits<uint16_t>::max();
       ++offset) {
    auto pfx = makePrefix(offset);
    if (!fib6->exactMatch(pfx)) {
      return pfx;
    }
  }
  CHECK(false) << " Should never get here";
}

void BaseEcmpResourceManagerTest::SetUp() {
  XLOG(DBG2) << "BaseEcmpResourceMgrTest SetUp";
  consolidator_ = makeResourceMgr();
  state_ = std::make_shared<SwitchState>();
  auto fibContainer =
      std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
  auto mfib = std::make_shared<MultiSwitchForwardingInformationBaseMap>();
  mfib->updateForwardingInformationBaseContainer(fibContainer, hwMatcher());
  state_->resetForwardingInformationBases(mfib);
  state_->publish();
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto pfx = makePrefix(i);
    auto route = makeRoute(pfx, defaultNhops());
    fib6->addNode(pfx.str(), std::move(route));
  }
  newState->publish();
  consolidate(newState);
  XLOG(DBG2) << "BaseEcmpResourceMgrTest SetUp done";
}

std::set<EcmpResourceManager::NextHopGroupId>
BaseEcmpResourceManagerTest::getNhopGroupIds() const {
  auto nhop2Id = consolidator_->getNhopsToId();
  std::set<NextHopGroupId> nhopIds;
  std::for_each(
      nhop2Id.begin(), nhop2Id.end(), [&nhopIds](const auto& nhopsAndId) {
        nhopIds.insert(nhopsAndId.second);
      });
  return nhopIds;
}

std::optional<EcmpResourceManager::NextHopGroupId>
BaseEcmpResourceManagerTest::getNhopId(const RouteNextHopSet& nhops) const {
  std::optional<EcmpResourceManager::NextHopGroupId> nhopId;
  auto nitr = consolidator_->getNhopsToId().find(nhops);
  if (nitr != consolidator_->getNhopsToId().end()) {
    nhopId = nitr->second;
  }
  return nhopId;
}

TEST_F(BaseEcmpResourceManagerTest, noFibsDelta) {
  auto oldState = state_;
  auto newState = oldState->clone();
  newState->getPorts()->modify(&newState);
  registerPort(newState, PortID(1), "portOne", hwMatcher());
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  EXPECT_EQ(deltas.begin()->oldState(), oldState);
  EXPECT_EQ(deltas.begin()->newState(), newState);
  EXPECT_NE(
      deltas.begin()->newState()->getPorts()->getPortIf("portOne"), nullptr);
}
} // namespace facebook::fboss
