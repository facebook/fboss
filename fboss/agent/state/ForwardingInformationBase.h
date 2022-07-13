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

template <typename AddressT>
using ForwardingInformationBaseTraits = NodeMapTraits<
    RoutePrefix<AddressT>,
    Route<AddressT>,
    NodeMapNoExtraFields,
    std::map<RoutePrefix<AddressT>, std::shared_ptr<Route<AddressT>>>>;

template <typename AddrT>
struct ForwardingInformationBaseThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::RouteFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "prefix";
    return _key;
  }

  static const std::string parseKey(const folly::dynamic& key) {
    return key.asString();
  }

  static std::string convertKey(const RoutePrefix<AddrT>& prefix) {
    return prefix.str();
  }
};

template <typename AddressT>
class ForwardingInformationBase
    : public ThriftyNodeMapT<
          ForwardingInformationBase<AddressT>,
          ForwardingInformationBaseTraits<AddressT>,
          ForwardingInformationBaseThriftTraits<AddressT>> {
 public:
  ForwardingInformationBase();
  ~ForwardingInformationBase() override;

  using Base = ThriftyNodeMapT<
      ForwardingInformationBase<AddressT>,
      ForwardingInformationBaseTraits<AddressT>,
      ForwardingInformationBaseThriftTraits<AddressT>>;

  std::shared_ptr<Route<AddressT>> exactMatch(
      const RoutePrefix<AddressT>& prefix) const;
  std::shared_ptr<Route<AddressT>> getRouteIf(
      const RoutePrefix<AddressT>& prefix) const {
    return exactMatch(prefix);
  }

  ForwardingInformationBase* modify(
      RouterID rid,
      std::shared_ptr<SwitchState>* state);

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
