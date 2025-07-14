// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/PreUpdateStateModifier.h"

namespace facebook::fboss {
class StateDelta;
class SwitchState;

class ShelManager : public PreUpdateStateModifier {
 public:
  std::vector<StateDelta> modifyState(
      const std::vector<StateDelta>& deltas) override;
  void updateDone() override;
  void updateFailed(const std::shared_ptr<SwitchState>& curState) override;

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

  std::unordered_map<InterfaceID, uint64_t> intf2RefCnt_;
  std::unordered_map<InterfaceID, uint64_t> preUpdateIntf2RefCnt_;
};

} // namespace facebook::fboss
