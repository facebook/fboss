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

#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <folly/dynamic.h>
#include <memory>

namespace facebook::fboss {

USE_THRIFT_COW(ForwardingInformationBaseContainer)
RESOLVE_STRUCT_MEMBER(
    ForwardingInformationBaseContainer,
    switch_state_tags::fibV4,
    ForwardingInformationBaseV4)
RESOLVE_STRUCT_MEMBER(
    ForwardingInformationBaseContainer,
    switch_state_tags::fibV6,
    ForwardingInformationBaseV6)

class ForwardingInformationBaseContainer
    : public ThriftStructNode<
          ForwardingInformationBaseContainer,
          state::FibContainerFields> {
 public:
  using Base = ThriftStructNode<
      ForwardingInformationBaseContainer,
      state::FibContainerFields>;
  explicit ForwardingInformationBaseContainer(RouterID vrf);
  ~ForwardingInformationBaseContainer() override;

  RouterID getID() const;
  const std::shared_ptr<ForwardingInformationBaseV4>& getFibV4() const;
  const std::shared_ptr<ForwardingInformationBaseV6>& getFibV6() const;

  template <typename AddressT>
  const std::shared_ptr<ForwardingInformationBase<AddressT>>& getFib() const {
    if constexpr (std::is_same_v<folly::IPAddressV6, AddressT>) {
      return getFibV6();
    } else {
      return getFibV4();
    }
  }
  template <typename AddressT>
  void setFib(const std::shared_ptr<ForwardingInformationBase<AddressT>>& fib) {
    if constexpr (std::is_same_v<folly::IPAddressV6, AddressT>) {
      this->ref<switch_state_tags::fibV6>() = fib;
    } else {
      this->ref<switch_state_tags::fibV4>() = fib;
    }
  }

  ForwardingInformationBaseContainer* modify(
      std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
