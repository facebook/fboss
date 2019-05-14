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
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeMap.h"

namespace facebook {
namespace fboss {
namespace rib {

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
      std::shared_ptr<ForwardingInformationBaseV4>(
          createUpdatedFib(v4NetworkToRoute_));
  nextFibContainer->writableFields()->fibV6 =
      std::shared_ptr<ForwardingInformationBaseV6>(
          createUpdatedFib(v6NetworkToRoute_));

  return nextState;
}

template <typename AddressT>
std::unique_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
ForwardingInformationBaseUpdater::createUpdatedFib(
    const facebook::fboss::rib::NetworkToRouteMap<AddressT>& ribRange) {
  typename facebook::fboss::ForwardingInformationBase<
      AddressT>::Base::NodeContainer updatedFib;

  for (const auto& ribRouteIt : ribRange) {
    const facebook::fboss::rib::Route<AddressT>& ribRoute = ribRouteIt->value();

    if (!ribRoute.isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }

    std::unique_ptr<facebook::fboss::Route<AddressT>> fibRoute =
        ribRoute.toFibRoute();

    auto prefix = fibRoute->prefix();
    updatedFib.emplace(prefix, fibRoute.release());
  }

  return std::make_unique<ForwardingInformationBase<AddressT>>(updatedFib);
}

} // namespace rib
} // namespace fboss
} // namespace facebook
