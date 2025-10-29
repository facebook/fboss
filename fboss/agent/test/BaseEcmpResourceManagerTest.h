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
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;
auto constexpr kClientID(ClientID::BGPD);

RouteNextHopSet
makeNextHops(int n, int numNhopsPerIntf = 1, int startOffset = 0);
RouteNextHopSet makeV4NextHops(int n);
RouteV6::Prefix makePrefix(int offset);
RouteV4::Prefix makeV4Prefix(int offset);

std::shared_ptr<RouteV6> makeRoute(
    const RouteV6::Prefix& pfx,
    const RouteNextHopSet& nextHops);

std::shared_ptr<RouteV4> makeV4Route(
    const RouteV4::Prefix& pfx,
    const RouteNextHopSet& nextHops);

template <typename AddrT>
inline ForwardingInformationBase<AddrT>* fibImpl(
    std::shared_ptr<SwitchState>& newState) {
  return newState->getFibs()
      ->getNode(RouterID(0))
      ->getFib<AddrT>()
      ->modify(RouterID(0), &newState);
}

inline ForwardingInformationBaseV6* fib(
    std::shared_ptr<SwitchState>& newState) {
  return fibImpl<folly::IPAddressV6>(newState);
}

inline ForwardingInformationBaseV4* fib4(
    std::shared_ptr<SwitchState>& newState) {
  return fibImpl<folly::IPAddressV4>(newState);
}

inline const std::shared_ptr<ForwardingInformationBaseV6> cfib(
    const std::shared_ptr<SwitchState>& newState) {
  return newState->getFibs()->getNode(RouterID(0))->getFibV6();
}

inline const std::shared_ptr<ForwardingInformationBaseV4> cfib4(
    const std::shared_ptr<SwitchState>& newState) {
  return newState->getFibs()->getNode(RouterID(0))->getFibV4();
}

inline HwSwitchMatcher hwMatcher() {
  return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
}

cfg::SwitchConfig onePortPerIntfConfig(
    int numIntfs,
    std::optional<cfg::SwitchingMode> backupSwitchingMode =
        cfg::SwitchingMode::PER_PACKET_RANDOM,
    int32_t ecmpCompressionThresholdPct = 0);

class BaseEcmpResourceManagerTest : public ::testing::Test {
 public:
  static constexpr auto kNumIntfs = 20;
  RouteNextHopSet defaultNhops() const {
    return makeNextHops(kNumIntfs);
  }
  RouteNextHopSet defaultV4Nhops() const {
    return makeV4NextHops(kNumIntfs);
  }
  using NextHopGroupId = EcmpResourceManager::NextHopGroupId;
  std::vector<StateDelta> consolidate(
      const std::shared_ptr<SwitchState>& state);
  void failUpdate(const std::shared_ptr<SwitchState>& state);
  void failUpdate(
      const std::shared_ptr<SwitchState>& state,
      const std::shared_ptr<SwitchState>& failTo);
  RouteV6::Prefix nextPrefix() const;
  void SetUp() override;
  void TearDown() override;
  std::set<NextHopGroupId> getNhopGroupIds() const;
  std::optional<EcmpResourceManager::NextHopGroupId> getNhopId(
      const RouteNextHopSet& nhops) const;
  virtual std::shared_ptr<EcmpResourceManager> makeResourceMgr() const;
  std::shared_ptr<EcmpResourceManager> makeResourceMgrWithEcmpLimit(
      int ecmpGroupLimit) const;
  virtual int numStartRoutes() const {
    return 5;
  }
  void assertDeltasForOverflow(const std::vector<StateDelta>& deltas) const;
  void assertRibFibEquivalence() const;
  std::vector<std::shared_ptr<RouteV6>> getPostConfigResolvedRoutes(
      const std::shared_ptr<SwitchState>& in) const;
  size_t numPostConfigResolvedRoutes(
      const std::shared_ptr<SwitchState>& in) const {
    return getPostConfigResolvedRoutes(in).size();
  }
  void verifyRouteUsageCount(
      const RouteNextHopSet& nhops,
      size_t expectedCount) {
    auto nhopId = getNhopId(nhops);
    ASSERT_TRUE(nhopId.has_value());
    EXPECT_EQ(consolidator_->getRouteUsageCount(nhopId.value()), expectedCount);
  }
  RouteNextHopSet getNextHops(EcmpResourceManager::NextHopGroupId gid) const;
  std::vector<StateDelta> addRoute(
      const RoutePrefixV6& prefix6,
      const RouteNextHopSet& nhops) {
    return addOrUpdateRoute(prefix6, nhops);
  }
  std::vector<StateDelta> addRoutes(
      const std::map<RoutePrefixV6, RouteNextHopSet>& route2Nhops) {
    return addOrUpdateRoutes(route2Nhops);
  }
  std::vector<StateDelta> updateRoute(
      const RoutePrefixV6& prefix6,
      const RouteNextHopSet& nhops) {
    return addOrUpdateRoute(prefix6, nhops);
  }
  std::vector<StateDelta> rmRoutes(const std::vector<RoutePrefixV6>& prefix6s);
  std::vector<StateDelta> rmRoute(const RoutePrefixV6& prefix6) {
    return rmRoutes({prefix6});
  }
  std::unique_ptr<std::vector<UnicastRoute>> getClientRoutes(
      ClientID client) const;
  void syncFib();
  void replayAllRoutesViaThrift();

  void assertTargetState(
      const std::shared_ptr<SwitchState>& targetState,
      const std::shared_ptr<SwitchState>& endStatePrefixes,
      const std::set<RouteV6::Prefix>& overflowPrefixes,
      const EcmpResourceManager* consolidatorToCheck = nullptr,
      bool checkStats = true);
  void assertEndState(
      const std::shared_ptr<SwitchState>& endStatePrefixes,
      const std::set<RouteV6::Prefix>& overflowPrefixes) {
    assertTargetState(state_, endStatePrefixes, overflowPrefixes);
  }
  std::set<RouteV6::Prefix> getPrefixesForGroups(
      const EcmpResourceManager::NextHopGroupIds& grpIds) const;

  std::set<RouteV6::Prefix> getPrefixesWithoutOverrides() const;
  EcmpResourceManager::NextHopGroupIds getGroupsWithoutOverrides() const;
  EcmpResourceManager::NextHopGroupIds getAllGroups() const;

 private:
  void assertReplayIsNoOp(bool syncFib);
  std::vector<StateDelta> addOrUpdateRoute(
      const RoutePrefixV6& prefix6,
      const RouteNextHopSet& nhops) {
    std::map<RoutePrefixV6, RouteNextHopSet> prefix2Nhops;
    prefix2Nhops.insert({prefix6, nhops});
    return addOrUpdateRoutes(prefix2Nhops);
  }
  std::vector<StateDelta> addOrUpdateRoutes(
      const std::map<RoutePrefixV6, RouteNextHopSet>& prefix2Nhops);
  virtual void setupFlags() const;

 public:
  void assertMergedGroup(
      const EcmpResourceManager::NextHopGroupIds& mergedGroup) const;
  void assertGroupsAreUnMerged(
      const EcmpResourceManager::NextHopGroupIds& unmergedGroups) const;
  int32_t virtual getEcmpCompressionThresholdPct() const {
    return 0;
  }
  virtual std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode() const {
    return cfg::SwitchingMode::PER_PACKET_RANDOM;
  }
  void updateFlowletSwitchingConfig(
      const std::shared_ptr<SwitchState>& newState);
  void updateRoutes(const std::shared_ptr<SwitchState>& newState);
  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<EcmpResourceManager> consolidator_;
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};
} // namespace facebook::fboss
