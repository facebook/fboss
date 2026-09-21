// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <gtest/gtest.h>

#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class StateDelta;
class SwitchState;
template <typename AddrT>
class Route;

class ShelManager {
 public:
  std::vector<StateDelta> modifyState(const std::vector<StateDelta>& deltas);
  std::vector<StateDelta> reconstructFromSwitchState(
      const std::shared_ptr<SwitchState>& curState);
  void updateDone();
  void updateFailed(const std::shared_ptr<SwitchState>& curState);
  bool ecmpOverShelDisabledPort(
      const std::map<int, cfg::PortState>& sysPortShelState);

 private:
  void updateRefCount(
      const RouteNextHopEntry::NextHopSet& routeNhops,
      const std::shared_ptr<SwitchState>& origState,
      bool add);
  template <typename AddrT>
  void routeAdded(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::shared_ptr<SwitchState>& origState);
  template <typename AddrT>
  void routeDeleted(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& removedRoute,
      const std::shared_ptr<SwitchState>& origState);
  void processRouteUpdates(const StateDelta& delta);
  std::shared_ptr<SwitchState> processDelta(const StateDelta& delta);
  std::vector<StateDelta> modifyStateImpl(
      const std::vector<StateDelta>& deltas);

  folly::Synchronized<std::unordered_map<InterfaceID, uint64_t>> intf2RefCnt_;

  FRIEND_TEST(ShelManagerTest, RefCountAndIntf2AddDel);
  FRIEND_TEST(ShelManagerTest, EcmpOverShelDisabledPort);
};

} // namespace facebook::fboss
