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

struct ForwardingInformationBaseContainerFields
    : public ThriftyFields<
          ForwardingInformationBaseContainerFields,
          state::FibContainerFields> {
  explicit ForwardingInformationBaseContainerFields(RouterID vrf);

  template <typename Fn>
  void forEachChild(Fn fn) {
    fn(fibV4.get());
    fn(fibV6.get());
  }

  folly::dynamic toFollyDynamicLegacy() const;
  static ForwardingInformationBaseContainerFields fromFollyDynamicLegacy(
      const folly::dynamic& dyn);

  state::FibContainerFields toThrift() const override;
  static ForwardingInformationBaseContainerFields fromThrift(
      state::FibContainerFields const& fields);

  bool operator==(const ForwardingInformationBaseContainerFields& other) const {
    ForwardingInformationBaseV4 emptyV4{};
    ForwardingInformationBaseV6 emptyV6{};
    auto fibV4Ptr = fibV4.get() ? fibV4.get() : &emptyV4;
    auto fibV6Ptr = fibV6.get() ? fibV6.get() : &emptyV6;
    auto otherFibV4Ptr = other.fibV4.get() ? other.fibV4.get() : &emptyV4;
    auto otherFibV6Ptr = other.fibV6.get() ? other.fibV6.get() : &emptyV6;
    return (vrf == other.vrf) &&
        (fibV4Ptr->toThrift() == otherFibV4Ptr->toThrift()) &&
        (fibV6Ptr->toThrift() == otherFibV6Ptr->toThrift());
  }

  bool operator!=(const ForwardingInformationBaseContainerFields& other) const {
    return !(*this == other);
  }

  RouterID vrf{0};
  std::shared_ptr<ForwardingInformationBaseV4> fibV4{nullptr};
  std::shared_ptr<ForwardingInformationBaseV6> fibV6{nullptr};
};

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
  using LegacyFields = ForwardingInformationBaseContainerFields;
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

  static std::shared_ptr<ForwardingInformationBaseContainer>
  fromFollyDynamicLegacy(const folly::dynamic& json);
  folly::dynamic toFollyDynamicLegacy() const;
  static std::shared_ptr<ForwardingInformationBaseContainer> fromFollyDynamic(
      const folly::dynamic& json);
  folly::dynamic toFollyDynamic() const override;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
