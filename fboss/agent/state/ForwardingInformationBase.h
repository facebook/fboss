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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/Thrifty.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace facebook::fboss {

using ForwardingInformationBaseClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using ForwardingInformationBaseType = std::map<std::string, state::RouteFields>;

template <typename AddrT>
class ForwardingInformationBase;

template <typename AddrT>
using ForwardingInformationBaseTraits = ThriftMapNodeTraits<
    ForwardingInformationBase<AddrT>,
    ForwardingInformationBaseClass,
    ForwardingInformationBaseType,
    Route<AddrT>>;

template <typename AddressT>
class ForwardingInformationBase
    : public ThriftMapNode<
          ForwardingInformationBase<AddressT>,
          ForwardingInformationBaseTraits<AddressT>> {
 public:
  ForwardingInformationBase() = default;
  virtual ~ForwardingInformationBase() override = default;

  using Base = ThriftMapNode<
      ForwardingInformationBase<AddressT>,
      ForwardingInformationBaseTraits<AddressT>>;
  using Base::modify;

  std::shared_ptr<Route<AddressT>> exactMatch(
      const RoutePrefix<AddressT>& prefix) const;
  std::shared_ptr<Route<AddressT>> getRouteIf(
      const RoutePrefix<AddressT>& prefix) const {
    return exactMatch(prefix);
  }

  ForwardingInformationBase* modify(
      RouterID rid,
      std::shared_ptr<SwitchState>* state);

  void disableTTLDecrement(const folly::IPAddress& addr) {
    setDisableTTLDecrement(addr, true);
  }

  void enableTTLDecrement(const folly::IPAddress& addr) {
    setDisableTTLDecrement(addr, false);
  }

 private:
  void setDisableTTLDecrement(const folly::IPAddress& addr, bool disable);

  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using ForwardingInformationBaseV4 =
    ForwardingInformationBase<folly::IPAddressV4>;
using ForwardingInformationBaseV6 =
    ForwardingInformationBase<folly::IPAddressV6>;

} // namespace facebook::fboss
