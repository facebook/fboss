// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Vlan.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

class SwSwitch;
class ResolvedNextHopProbe;

class ResolvedNexthopProbeScheduler {
  /*
   * manages probes to l3 resolved next hops, for every route delta, resolved
   * next hop monitor triggers scheduler a probe is removed if no route
   * references resolved next hop a probe is added if no probe exists to that
   * resolved next hop
   */
 public:
  explicit ResolvedNexthopProbeScheduler(SwSwitch* sw);
  ~ResolvedNexthopProbeScheduler();
  void processChangedResolvedNexthops(
      std::vector<ResolvedNextHop> added,
      std::vector<ResolvedNextHop> removed);

  boost::container::flat_map<ResolvedNextHop, uint32_t>
  resolvedNextHop2UseCount() const {
    return resolvedNextHop2UseCount_;
  }

  const boost::container::
      flat_map<ResolvedNextHop, std::shared_ptr<ResolvedNextHopProbe>>&
      resolvedNextHop2Probes() const {
    return resolvedNextHop2Probes_;
  }

  void schedule();

 private:
  template <typename VlanOrIntfT>
  bool shouldProbe(
      const folly::IPAddress& addr,
      const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
    if (addr.isV4()) {
      auto table =
          vlanOrIntf->template getNeighborEntryTable<folly::IPAddressV4>();
      return table->getEntryIf(addr.asV4()) == nullptr;
    } else {
      auto table =
          vlanOrIntf->template getNeighborEntryTable<folly::IPAddressV6>();
      return table->getEntryIf(addr.asV6()) == nullptr;
    }
  }

  SwSwitch* sw_{nullptr};
  boost::container::
      flat_map<ResolvedNextHop, std::shared_ptr<ResolvedNextHopProbe>>
          resolvedNextHop2Probes_;
  boost::container::flat_map<ResolvedNextHop, uint32_t>
      resolvedNextHop2UseCount_;
};

} // namespace facebook::fboss
