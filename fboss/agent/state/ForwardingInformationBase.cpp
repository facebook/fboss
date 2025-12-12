/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBase.h"

#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template <typename AddressT>
std::shared_ptr<Route<AddressT>>
ForwardingInformationBase<AddressT>::exactMatch(
    const RoutePrefix<AddressT>& prefix) const {
  return ForwardingInformationBase::Base::getNodeIf(prefix.str());
}

template <typename AddressT>
ForwardingInformationBase<AddressT>* ForwardingInformationBase<
    AddressT>::modify(RouterID rid, std::shared_ptr<SwitchState>* state) {
  if (!this->isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }
  auto fibContainer =
      (*state)->cref<switch_state_tags::fibsMap>()->getNode(rid)->modify(state);
  auto clonedFib = this->clone();
  fibContainer->setFib<AddressT>(clonedFib);
  return clonedFib.get();
}

template <typename AddressT>
void ForwardingInformationBase<AddressT>::setDisableTTLDecrement(
    const folly::IPAddress& addr,
    bool disable) {
  /* any future routes to same next hop may have their TTL decrement */
  CHECK(!this->isPublished());
  for (auto& entry : *this) {
    auto route = entry.second;
    auto adminDistance = route->getForwardInfo().getAdminDistance();
    auto counter = route->getForwardInfo().getCounterID();
    auto classID = route->getForwardInfo().getClassID();
    RouteNextHopEntry::NextHopSet nhops =
        route->getForwardInfo().getNextHopSet();
    bool changed = false;
    for (auto& nhop : nhops) {
      CHECK(nhop.isResolved());
      if (nhop.addr() == addr) {
        changed = true;
        folly::poly_cast<ResolvedNextHop>(nhop).setDisableTTLDecrement(disable);
      }
    }
    if (changed) {
      route = route->clone();
      route->setResolved(
          RouteNextHopEntry(nhops, adminDistance, counter, classID));
      entry.second = route;
    }
  }
}

template struct ThriftMapNode<
    ForwardingInformationBase<folly::IPAddressV4>,
    ForwardingInformationBaseTraits<folly::IPAddressV4>>;
template struct ThriftMapNode<
    ForwardingInformationBase<folly::IPAddressV6>,
    ForwardingInformationBaseTraits<folly::IPAddressV6>>;
template class ForwardingInformationBase<folly::IPAddressV4>;
template class ForwardingInformationBase<folly::IPAddressV6>;

} // namespace facebook::fboss
