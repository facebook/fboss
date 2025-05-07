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

namespace facebook::fboss {

RouteNextHopSet makeNextHops(int n) {
  CHECK_LT(n, 255);
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    std::stringstream ss;
    ss << std::hex << i + 1;
    auto ipStr = "100::" + ss.str();
    h.emplace(UnresolvedNextHop(folly::IPAddress(ipStr), UCMP_DEFAULT_WEIGHT));
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
  return rt;
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::consolidate(
    const std::shared_ptr<SwitchState>& state) {
  StateDelta delta(state_, state);
  auto deltas = consolidator_->consolidate(delta);
  if (deltas.size()) {
    assertDeltasForOverflow(deltas);
    state_ = deltas.back().newState();
  } else {
    state_ = state;
  }
  state_->publish();
  EXPECT_EQ(state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(), 0);
  consolidator_->updateDone(delta);
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
          route->getForwardInfo().getNextHopSet());
      if (pitr != primaryEcmpTypeGroups2RefCnt.end()) {
        ++pitr->second;
      } else {
        primaryEcmpTypeGroups2RefCnt.insert(
            {route->getForwardInfo().getNextHopSet(), 1});
      }
    }
  }
  // ECMP groups should not exceed the limit.
  EXPECT_LE(
      primaryEcmpTypeGroups2RefCnt.size(),
      consolidator_->getMaxPrimaryEcmpGroups());
  // TODO process each delta and assert that ECMP group limit is never
  // exceeded.
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
  consolidate(newState);
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
} // namespace facebook::fboss
