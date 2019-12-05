// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"

#include "fboss/agent/state/Route.h"

namespace facebook {
namespace fboss {

class SwSwitch;
class StateDelta;

class ResolvedNexthopMonitor : public AutoRegisterStateObserver {
  /*
   * under certain topologies and routing protocols, resolved l3 next hop may
   * not be l2 resolved i.e. a routing protocol may indicate next hop x is
   * reachable or directly connected via interface y, but arp/ndp mechanism may
   * never trigger to discover this next hop. arp/ndp mechanism triggers only
   * when packet directed towards neighbor is trapped in cpu and
   * solicitations/probes to that neighbor are generated. but if that next hop
   * is part of "multipath" next hop, it is "skipped" from getting programmed in
   * hardware and so packets to that next hop are never trapped. further routing
   * protocol agents may not use unicast communication method. this will leave
   * resolved l3 next hop's neighbor entries unresolved until there is a
   * specific trigger (often external such as ping) to initiate ndp/arp
   * discovery. to work around such cases, resolved next hop monitor schedules
   * probes to each resolved next hop. a probe gets  added when  a new resolved
   * l3 next hop gets added through either new or changed  resolved route is
   * added or changed. a probes gets removed when a resolved l3 next hop is no
   * longer referenced by any resolved route. a probe gets "scheduled" when
   * neighbor entry does not exist for corresponding l3 resolved next hop a
   * probe gets "unscheduled" when neighbor entry exists (even if pending) for
   * corresponding l3 resolved next hop
   * when a probe is scheduled it will keep generating arp or ndp requests to
   * corresponding next hop.
   */
 public:
  explicit ResolvedNexthopMonitor(SwSwitch* sw);
  void stateUpdated(const StateDelta& delta) override;

  bool probesScheduled() const {
    return scheduleProbes_;
  }

 private:
  template <typename RouteT>
  void processAddedRouteNextHops(const std::shared_ptr<RouteT>& route) {
    if (skipRoute(route)) {
      return;
    }
    const auto& fwd = route->getForwardInfo();
    for (auto nhop : fwd.normalizedNextHops()) {
      added_.emplace_back(nhop.addr(), nhop.intf(), 0);
    }
  }

  template <typename RouteT>
  void processRemovedRouteNextHops(const std::shared_ptr<RouteT>& route) {
    if (skipRoute(route)) {
      return;
    }
    const auto& fwd = route->getForwardInfo();
    for (auto nhop : fwd.normalizedNextHops()) {
      removed_.emplace_back(nhop.addr(), nhop.intf(), 0);
    }
  }

  template <typename RouteT>
  void processChangedRouteNextHops(
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute) {
    processRemovedRouteNextHops(oldRoute);
    processAddedRouteNextHops(newRoute);
  }

  template <typename RouteT>
  bool skipRoute(const std::shared_ptr<RouteT>& route) const {
    return !route->isResolved() ||
        route->getForwardInfo().getAction() !=
        RouteNextHopEntry::Action::NEXTHOPS;
  }

  SwSwitch* sw_{nullptr};
  std::vector<ResolvedNextHop> added_;
  std::vector<ResolvedNextHop> removed_;
  bool scheduleProbes_{false}; // trigger next hop probe scheduler
};

} // namespace fboss
} // namespace facebook
