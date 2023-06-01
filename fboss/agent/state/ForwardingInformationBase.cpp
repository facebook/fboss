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

template class ThriftMapNode<
    ForwardingInformationBase<folly::IPAddressV4>,
    ForwardingInformationBaseTraits<folly::IPAddressV4>>;
template class ThriftMapNode<
    ForwardingInformationBase<folly::IPAddressV6>,
    ForwardingInformationBaseTraits<folly::IPAddressV6>>;
template class ForwardingInformationBase<folly::IPAddressV4>;
template class ForwardingInformationBase<folly::IPAddressV6>;

} // namespace facebook::fboss
