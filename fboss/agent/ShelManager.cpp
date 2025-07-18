// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ShelManager.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::vector<StateDelta> ShelManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  // TODO: Handle list of deltas instead of single delta
  CHECK_EQ(deltas.size(), 1);
  preUpdateIntf2RefCnt_ = intf2RefCnt_;
  processRouteUpdates(*deltas.begin());

  // TODO(zecheng): implement this function
  std::vector<StateDelta> retDeltas;
  retDeltas.emplace_back(
      deltas.begin()->oldState(), deltas.begin()->newState());
  return retDeltas;
}

std::vector<StateDelta> ShelManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  intf2RefCnt_.clear();
  std::vector<StateDelta> deltas;
  deltas.emplace_back(std::make_shared<SwitchState>(), curState);
  return modifyStateImpl(deltas);
}

void ShelManager::updateDone() {
  // TODO(zecheng): implement this function
}
void ShelManager::updateFailed(
    const std::shared_ptr<SwitchState>& /*curState*/) {
  // TODO(zecheng): implement this function
}

void ShelManager::updateRefCount(
    const RouteNextHopEntry::NextHopSet& routeNhops,
    const std::shared_ptr<SwitchState>& origState,
    bool add) {
  for (const auto& nhop : routeNhops) {
    // NextHops that is resolved to local interfaces with global scope
    if (nhop.isResolved()) {
      auto sysPort =
          origState->getSystemPorts()->getNodeIf(SystemPortID(nhop.intf()));
      if (sysPort && sysPort->getScope() == cfg::Scope::GLOBAL) {
        auto iter = intf2RefCnt_.find(nhop.intf());
        if (add) {
          if (iter == intf2RefCnt_.end()) {
            intf2RefCnt_[nhop.intf()] = 1;
          } else {
            iter->second++;
          }
        } else {
          CHECK(iter != intf2RefCnt_.end() && iter->second > 0);
          iter->second--;
          if (iter->second == 0) {
            intf2RefCnt_.erase(iter);
          }
        }
      }
    }
  }
}

template <typename AddrT>
void ShelManager::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    const std::shared_ptr<SwitchState>& origState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(newRoute->isResolved());
  CHECK(newRoute->isPublished());
  const auto& routeNhops = newRoute->getForwardInfo().normalizedNextHops();
  if (routeNhops.size() > 1) {
    updateRefCount(routeNhops, origState, true /*add*/);
  }
}

template <typename AddrT>
void ShelManager::routeDeleted(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& removedRoute,
    const std::shared_ptr<SwitchState>& origState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(removedRoute->isResolved());
  CHECK(removedRoute->isPublished());
  const auto& routeNhops = removedRoute->getForwardInfo().normalizedNextHops();
  if (routeNhops.size() > 1) {
    updateRefCount(routeNhops, origState, false /*add*/);
  }
}

void ShelManager::processRouteUpdates(const StateDelta& delta) {
  processFibsDeltaInHwSwitchOrder(
      delta,
      [this, &delta](RouterID rid, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          routeDeleted(rid, oldRoute, delta.newState());
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          routeAdded(rid, newRoute, delta.newState());
          return;
        }
        // Both old and new are resolved
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        routeDeleted(rid, oldRoute, delta.newState());
        routeAdded(rid, newRoute, delta.newState());
      },
      [this, &delta](RouterID rid, const auto& newRoute) {
        if (newRoute->isResolved()) {
          routeAdded(rid, newRoute, delta.newState());
        }
      },
      [this, &delta](RouterID rid, const auto& oldRoute) {
        if (oldRoute->isResolved()) {
          routeDeleted(rid, oldRoute, delta.newState());
        }
      });
}

std::shared_ptr<SwitchState> ShelManager::processDelta(
    const StateDelta& delta) {
  auto beforeIntf2RefCnt = intf2RefCnt_;
  processRouteUpdates(delta);
  auto modifiedState = delta.newState()->clone();

  auto processIntfDiff =
      [&](const auto& fromMap, const auto& toMap, bool enable) {
        for (const auto& [intf, _] : toMap) {
          if (fromMap.find(intf) == fromMap.end()) {
            auto portId =
                getPortID(static_cast<SystemPortID>(intf), delta.newState());
            auto port = modifiedState->getPorts()->getNodeIf(portId);
            CHECK(port);
            auto desiredShelState = port->getDesiredSelfHealingECMPLagEnable();
            if (desiredShelState.has_value() && desiredShelState.value()) {
              auto newPort = port->modify(&modifiedState);
              newPort->setSelfHealingECMPLagEnable(enable);
            }
          }
        }
      };

  processIntfDiff(beforeIntf2RefCnt, intf2RefCnt_, true);
  processIntfDiff(intf2RefCnt_, beforeIntf2RefCnt, false);
  modifiedState->publish();
  return modifiedState;
}

std::vector<StateDelta> ShelManager::modifyStateImpl(
    const std::vector<StateDelta>& deltas) {
  std::vector<StateDelta> retDeltas;
  retDeltas.reserve(deltas.size());
  auto oldState = deltas.begin()->oldState();
  for (const auto& delta : deltas) {
    auto modifiedState = processDelta(StateDelta(oldState, delta.newState()));
    retDeltas.emplace_back(oldState, modifiedState);
    oldState = modifiedState;
  }
  return retDeltas;
}

} // namespace facebook::fboss
