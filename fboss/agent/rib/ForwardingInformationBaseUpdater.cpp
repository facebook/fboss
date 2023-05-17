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

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook::fboss {

ForwardingInformationBaseUpdater::ForwardingInformationBaseUpdater(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute)
    : resolver_(resolver),
      vrf_(vrf),
      v4NetworkToRoute_(v4NetworkToRoute),
      v6NetworkToRoute_(v6NetworkToRoute),
      labelToRoute_(labelToRoute) {}

std::shared_ptr<SwitchState> ForwardingInformationBaseUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  // A ForwardingInformationBaseContainer holds a
  // ForwardingInformationBaseV4 and a ForwardingInformationBaseV6 for a
  // particular VRF. Since FIBs for both address families will be updated,
  // we invoke modify() on the ForwardingInformationBaseContainer rather
  // than on its two children (namely ForwardingInformationBaseV4 and
  // ForwardingInformationBaseV6) in succession.
  // Unlike the coupled RIB implementation, we need only update the
  // SwitchState for a single VRF.
  std::shared_ptr<SwitchState> nextState(state);
  auto previousFibContainer = nextState->getFibs()->getNodeIf(vrf_);
  if (!previousFibContainer) {
    auto fibMap = nextState->getFibs()->modify(&nextState);
    previousFibContainer =
        std::make_shared<ForwardingInformationBaseContainer>(vrf_);
    fibMap->updateForwardingInformationBaseContainer(
        previousFibContainer, resolver_->scope(previousFibContainer));
  }
  CHECK(previousFibContainer);
  auto newFibV4 =
      createUpdatedFib(v4NetworkToRoute_, previousFibContainer->getFibV4());

  auto newFibV6 =
      createUpdatedFib(v6NetworkToRoute_, previousFibContainer->getFibV6());

  auto newLabelFib = createUpdatedLabelFib(
      labelToRoute_, state->getLabelForwardingInformationBase());

  if (!newFibV4 && !newFibV6 && !newLabelFib) {
    // return nextState in case we modified state above to insert new VRF
    return nextState;
  }
  auto nextFibContainer = previousFibContainer->modify(&nextState);

  if (newFibV4) {
    nextFibContainer->ref<switch_state_tags::fibV4>() = std::move(newFibV4);
  }

  if (newFibV6) {
    nextFibContainer->ref<switch_state_tags::fibV6>() = std::move(newFibV6);
  }

  if (newLabelFib) {
    nextState->resetLabelForwardingInformationBase(newLabelFib);
  }
  return nextState;
}

template <typename AddressT>
std::shared_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
ForwardingInformationBaseUpdater::createUpdatedFib(
    const facebook::fboss::NetworkToRouteMap<AddressT>& rib,
    const std::shared_ptr<facebook::fboss::ForwardingInformationBase<AddressT>>&
        fib) {
  typename facebook::fboss::ForwardingInformationBase<
      AddressT>::Base::NodeContainer updatedFib;

  bool updated = false;
  for (const auto& entry : rib) {
    const auto& ribRoute = entry.value();

    if (!ribRoute->isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }

    // TODO(samank): optimize to linear time intersection algorithm
    facebook::fboss::RoutePrefix<AddressT> fibPrefix{
        ribRoute->prefix().network(), ribRoute->prefix().mask()};
    std::shared_ptr<facebook::fboss::Route<AddressT>> fibRoute =
        fib->getNodeIf(fibPrefix.str());
    if (fibRoute) {
      if (fibRoute == ribRoute || fibRoute->isSame(ribRoute.get())) {
        // Pointer or contents are same, reuse existing route
      } else {
        fibRoute = ribRoute;
        updated = true;
      }
    } else {
      // new route
      fibRoute = ribRoute;
      updated = true;
    }
    CHECK(fibRoute->isPublished());
    updatedFib.emplace_hint(updatedFib.cend(), fibPrefix.str(), fibRoute);
  }
  // Check for deleted routes. Routes that were in the previous FIB
  // and have now been removed
  for (const auto& iter : std::as_const(*fib)) {
    const auto& fibEntry = iter.second;
    auto prefix = fibEntry->getID();
    if (updatedFib.find(prefix) == updatedFib.end()) {
      updated = true;
      break;
    }
  }

  DCHECK_EQ(
      updatedFib.size(),
      std::count_if(
          rib.begin(),
          rib.end(),
          [](const typename std::remove_reference_t<
              decltype(rib)>::ConstIterator::TreeNode& entry) {
            return entry.value()->isResolved();
          }));

  return updated ? std::make_shared<ForwardingInformationBase<AddressT>>(
                       std::move(updatedFib))
                 : nullptr;
}

std::shared_ptr<facebook::fboss::LabelForwardingInformationBase>
ForwardingInformationBaseUpdater::createUpdatedLabelFib(
    const facebook::fboss::NetworkToRouteMap<LabelID>& rib,
    std::shared_ptr<facebook::fboss::LabelForwardingInformationBase> fib) {
  if (!FLAGS_mpls_rib) {
    return nullptr;
  }

  bool updated = false;
  auto newFib = std::make_shared<LabelForwardingInformationBase>();
  for (const auto& entry : rib) {
    const auto& label = entry.first;
    const auto& ribRoute = entry.second;
    if (!ribRoute->isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }
    auto fibRoute = fib->getNodeIf(label);
    if (fibRoute) {
      if (fibRoute == ribRoute || fibRoute->isSame(ribRoute.get())) {
        // Pointer or contents are same, reuse existing route
      } else {
        fibRoute = ribRoute;
        updated = true;
      }
    } else {
      fibRoute = ribRoute;
      updated = true;
    }
    if (!facebook::fboss::MultiLabelForwardingInformationBase::
            isValidNextHopSet(ribRoute->getForwardInfo().getNextHopSet())) {
      throw FbossError("invalid label next hop");
    }
    CHECK(fibRoute->isPublished());
    newFib->addNode(fibRoute);
  }
  // Check for deleted routes. Routes that were in the previous FIB
  // and have now been removed
  for (const auto& iter : std::as_const(*fib)) {
    const auto& fibEntry = iter.second;
    const auto& label = fibEntry->getID();
    if (!newFib->getNodeIf(label)) {
      updated = true;
      break;
    }
  }
  return updated ? newFib : nullptr;
}

} // namespace facebook::fboss
