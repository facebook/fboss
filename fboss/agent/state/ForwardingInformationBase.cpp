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

namespace facebook::fboss {

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

template <typename AddressT>
std::shared_ptr<Route<AddressT>>
ForwardingInformationBase<AddressT>::longestMatch(
    const AddressT& address) const {
  std::shared_ptr<Route<AddressT>> longestMatchRoute = nullptr;
  // longestCommonLength must be wider than int8_t because it needs to hold
  // values in the range [-1, 128].
  int16_t longestCommonLength = -1;

  uint8_t currentCommonLength;
  for (const auto& prefixAndRoute : Base::getAllNodes()) {
    const auto& currentNetwork = prefixAndRoute.first.network;
    const auto currentMask = prefixAndRoute.first.mask;

    if (!address.inSubnet(currentNetwork, currentMask)) {
      continue;
    }

    auto currentCommonPrefixAndLength = folly::IPAddress::longestCommonPrefix(
        {address, address.bitCount()},
        {prefixAndRoute.first.network, prefixAndRoute.first.mask});

    currentCommonLength = currentCommonPrefixAndLength.second;
    if (currentCommonLength > longestCommonLength) {
      longestCommonLength = currentCommonLength;
      longestMatchRoute = prefixAndRoute.second;
    }
  }

  return longestMatchRoute;
}

FBOSS_INSTANTIATE_NODE_MAP(
    ForwardingInformationBase<folly::IPAddressV4>,
    ForwardingInformationBaseTraits<folly::IPAddressV4>);
FBOSS_INSTANTIATE_NODE_MAP(
    ForwardingInformationBase<folly::IPAddressV6>,
    ForwardingInformationBaseTraits<folly::IPAddressV6>);
template class ForwardingInformationBase<folly::IPAddressV4>;
template class ForwardingInformationBase<folly::IPAddressV6>;

} // namespace facebook::fboss
