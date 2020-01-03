/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace facebook::fboss {

template <typename AddressT>
using ForwardingInformationBaseTraits =
    NodeMapTraits<RoutePrefix<AddressT>, Route<AddressT>>;

template <typename AddressT>
class ForwardingInformationBase
    : public NodeMapT<
          ForwardingInformationBase<AddressT>,
          ForwardingInformationBaseTraits<AddressT>> {
 public:
  ForwardingInformationBase();
  ~ForwardingInformationBase() override;

  using Base = NodeMapT<
      ForwardingInformationBase<AddressT>,
      ForwardingInformationBaseTraits<AddressT>>;

  std::shared_ptr<Route<AddressT>> exactMatch(
      const RoutePrefix<AddressT>& prefix) const;

  std::shared_ptr<Route<AddressT>> longestMatch(const AddressT& address) const;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using ForwardingInformationBaseV4 =
    ForwardingInformationBase<folly::IPAddressV4>;
using ForwardingInformationBaseV6 =
    ForwardingInformationBase<folly::IPAddressV6>;

} // namespace facebook::fboss
