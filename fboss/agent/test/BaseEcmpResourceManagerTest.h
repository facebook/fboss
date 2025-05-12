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
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;

RouteNextHopSet makeNextHops(int n);
RouteV6::Prefix makePrefix(int offset);

std::shared_ptr<RouteV6> makeRoute(
    const RouteV6::Prefix& pfx,
    const RouteNextHopSet& nextHops);

inline ForwardingInformationBaseV6* fib(
    std::shared_ptr<SwitchState>& newState) {
  return newState->getFibs()
      ->getNode(RouterID(0))
      ->getFibV6()
      ->modify(RouterID(0), &newState);
}
inline const std::shared_ptr<ForwardingInformationBaseV6> cfib(
    const std::shared_ptr<SwitchState>& newState) {
  return newState->getFibs()->getNode(RouterID(0))->getFibV6();
}

inline HwSwitchMatcher hwMatcher() {
  return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
}

class BaseEcmpResourceManagerTest : public ::testing::Test {
 public:
  RouteNextHopSet defaultNhops() const {
    return makeNextHops(54);
  }
  using NextHopGroupId = EcmpResourceManager::NextHopGroupId;
  std::vector<StateDelta> consolidate(
      const std::shared_ptr<SwitchState>& state);
  RouteV6::Prefix nextPrefix() const;
  void SetUp() override;
  std::set<NextHopGroupId> getNhopGroupIds() const;
  std::optional<EcmpResourceManager::NextHopGroupId> getNhopId(
      const RouteNextHopSet& nhops) const;
  virtual std::shared_ptr<EcmpResourceManager> makeResourceMgr() const {
    static constexpr auto kEcmpGroupHwLimit = 100;
    return std::make_shared<EcmpResourceManager>(kEcmpGroupHwLimit);
  };
  virtual int numStartRoutes() const {
    return 10;
  }
  void assertDeltasForOverflow(const std::vector<StateDelta>& deltas) const;
  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<EcmpResourceManager> consolidator_;
};
} // namespace facebook::fboss
