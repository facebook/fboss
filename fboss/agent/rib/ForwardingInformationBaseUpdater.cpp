/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"

#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook::fboss::rib {

ForwardingInformationBaseUpdater::ForwardingInformationBaseUpdater(
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute)
    : vrf_(vrf),
      v4NetworkToRoute_(v4NetworkToRoute),
      v6NetworkToRoute_(v6NetworkToRoute) {}

std::shared_ptr<SwitchState> ForwardingInformationBaseUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  std::shared_ptr<SwitchState> nextState(state);

  // A ForwardingInformationBaseContainer holds a
  // ForwardingInformationBaseV4 and a ForwardingInformationBaseV6 for a
  // particular VRF. Since FIBs for both address families will be updated,
  // we invoke modify() on the ForwardingInformationBaseContainer rather
  // than on its two children (namely ForwardingInformationBaseV4 and
  // ForwardingInformationBaseV6) in succession.
  // Unlike the coupled RIB implementation, we need only update the
  // SwitchState for a single VRF.
  auto previousFibContainer = state->getFibs()->getFibContainerIf(vrf_);
  CHECK(previousFibContainer);

  auto nextFibContainer = previousFibContainer->modify(&nextState);

  nextFibContainer->writableFields()->fibV4 =
      std::shared_ptr<ForwardingInformationBaseV4>(createUpdatedFib(
          v4NetworkToRoute_, previousFibContainer->getFibV4()));

  nextFibContainer->writableFields()->fibV6 =
      std::shared_ptr<ForwardingInformationBaseV6>(createUpdatedFib(
          v6NetworkToRoute_, previousFibContainer->getFibV6()));

  return nextState;
}

template <typename AddressT>
std::unique_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
ForwardingInformationBaseUpdater::createUpdatedFib(
    const facebook::fboss::rib::NetworkToRouteMap<AddressT>& rib,
    const std::shared_ptr<facebook::fboss::ForwardingInformationBase<AddressT>>&
        fib) {
  // TODO(samank): updateFib should have size equal to the number of resovled
  // routes in the rib

  typename facebook::fboss::ForwardingInformationBase<
      AddressT>::Base::NodeContainer updatedFib;

  for (const auto& entry : rib) {
    const facebook::fboss::rib::Route<AddressT>& ribRoute = entry.value();

    if (!ribRoute.isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }

    // TODO(samank): optimize to linear time intersection algorithm
    facebook::fboss::RoutePrefix<AddressT> fibPrefix{ribRoute.prefix().network,
                                                     ribRoute.prefix().mask};
    std::shared_ptr<facebook::fboss::Route<AddressT>> fibRoute =
        fib->getNodeIf(fibPrefix);
    if (fibRoute) {
      if (toFibNextHop(ribRoute.getForwardInfo()) ==
          fibRoute->getForwardInfo()) {
        // Reuse prior FIB route
      } else {
        fibRoute = toFibRoute(ribRoute);
      }
    } else {
      fibRoute = toFibRoute(ribRoute);
    }

    updatedFib.emplace_hint(updatedFib.cend(), fibPrefix, fibRoute);
  }

  DCHECK_EQ(
      updatedFib.size(),
      std::count_if(
          rib.begin(),
          rib.end(),
          [](const typename std::remove_reference_t<decltype(
                 rib)>::ConstIterator::TreeNode& entry) {
            return entry.value().isResolved();
          }));

  return std::make_unique<ForwardingInformationBase<AddressT>>(
      std::move(updatedFib));
}

facebook::fboss::RouteNextHopEntry
ForwardingInformationBaseUpdater::toFibNextHop(
    const RouteNextHopEntry& ribNextHopEntry) {
  switch (ribNextHopEntry.getAction()) {
    case facebook::fboss::rib::RouteNextHopEntry::Action::DROP:
      return facebook::fboss::RouteNextHopEntry(
          facebook::fboss::RouteNextHopEntry::Action::DROP,
          ribNextHopEntry.getAdminDistance());
    case facebook::fboss::rib::RouteNextHopEntry::Action::TO_CPU:
      return facebook::fboss::RouteNextHopEntry(
          facebook::fboss::RouteNextHopEntry::Action::TO_CPU,
          ribNextHopEntry.getAdminDistance());
    case facebook::fboss::rib::RouteNextHopEntry::Action::NEXTHOPS: {
      facebook::fboss::RouteNextHopEntry::NextHopSet fibNextHopSet;
      for (const auto& ribNextHop : ribNextHopEntry.getNextHopSet()) {
        fibNextHopSet.insert(facebook::fboss::ResolvedNextHop(
            ribNextHop.addr(),
            ribNextHop.intfID().value(),
            ribNextHop.weight()));
      }
      return facebook::fboss::RouteNextHopEntry(
          fibNextHopSet, ribNextHopEntry.getAdminDistance());
    }
  }

  XLOG(FATAL) << "Unknown RouteNextHopEntry::Action value";
}

template <typename AddrT>
std::unique_ptr<facebook::fboss::Route<AddrT>>
ForwardingInformationBaseUpdater::toFibRoute(const Route<AddrT>& ribRoute) {
  CHECK(ribRoute.isResolved());

  facebook::fboss::RoutePrefix<AddrT> fibPrefix;
  fibPrefix.network = ribRoute.prefix().network;
  fibPrefix.mask = ribRoute.prefix().mask;

  auto fibRoute = std::make_unique<facebook::fboss::Route<AddrT>>(fibPrefix);

  fibRoute->setResolved(toFibNextHop(ribRoute.getForwardInfo()));
  if (ribRoute.isConnected()) {
    fibRoute->setConnected();
  }

  return fibRoute;
}

template std::unique_ptr<facebook::fboss::Route<folly::IPAddressV4>>
ForwardingInformationBaseUpdater::toFibRoute<folly::IPAddressV4>(
    const Route<folly::IPAddressV4>&);
template std::unique_ptr<facebook::fboss::Route<folly::IPAddressV6>>
ForwardingInformationBaseUpdater::toFibRoute<folly::IPAddressV6>(
    const Route<folly::IPAddressV6>&);

} // namespace facebook::fboss::rib
