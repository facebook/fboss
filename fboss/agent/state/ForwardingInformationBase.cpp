/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "ForwardingInformationBase.h"

#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook {
namespace fboss {

template <typename AddressT>
ForwardingInformationBase<AddressT>::ForwardingInformationBase() {}

template <typename AddressT>
ForwardingInformationBase<AddressT>::~ForwardingInformationBase() {}

template <typename AddressT>
std::shared_ptr<Route<AddressT>>
ForwardingInformationBase<AddressT>::exactMatch(
    const RoutePrefix<AddressT>& prefix) const {
  return ForwardingInformationBase::Base::getNodeIf(prefix);
}

FBOSS_INSTANTIATE_NODE_MAP(
    ForwardingInformationBase<folly::IPAddressV4>,
    ForwardingInformationBaseTraits<folly::IPAddressV4>);
FBOSS_INSTANTIATE_NODE_MAP(
    ForwardingInformationBase<folly::IPAddressV6>,
    ForwardingInformationBaseTraits<folly::IPAddressV6>);
template class ForwardingInformationBase<folly::IPAddressV4>;
template class ForwardingInformationBase<folly::IPAddressV6>;

} // namespace fboss
} // namespace facebook
