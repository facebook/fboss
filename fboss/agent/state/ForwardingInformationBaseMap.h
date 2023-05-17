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
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

#include <cstdint>
#include <memory>

namespace facebook::fboss {

class SwitchState;

using LegacyForwardingInformationBaseMapTraits =
    NodeMapTraits<RouterID, ForwardingInformationBaseContainer>;

using ForwardingInformationBaseMapClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using ForwardingInformationBaseMapThriftType =
    std::map<int16_t, state::FibContainerFields>;

class ForwardingInformationBaseMap;
using ForwardingInformationBaseMapTraits = ThriftMapNodeTraits<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseMapClass,
    ForwardingInformationBaseMapThriftType,
    ForwardingInformationBaseContainer>;

class ForwardingInformationBaseMap : public ThriftMapNode<
                                         ForwardingInformationBaseMap,
                                         ForwardingInformationBaseMapTraits> {
 public:
  using Traits = ForwardingInformationBaseMapTraits;
  using BaseT = ThriftMapNode<
      ForwardingInformationBaseMap,
      ForwardingInformationBaseMapTraits>;
  using BaseT::modify;
  ForwardingInformationBaseMap();
  ~ForwardingInformationBaseMap() override;

  std::pair<uint64_t, uint64_t> getRouteCount() const;

  std::shared_ptr<ForwardingInformationBaseContainer> getFibContainerIf(
      RouterID vrf) const;

  std::shared_ptr<ForwardingInformationBaseContainer> getFibContainer(
      RouterID vrf) const;

  ForwardingInformationBaseMap* modify(std::shared_ptr<SwitchState>* state);

  void updateForwardingInformationBaseContainer(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiSwitchForwardingInformationBaseMapTypeClass =
    apache::thrift::type_class::map<
        apache::thrift::type_class::string,
        ForwardingInformationBaseMapClass>;
using MultiSwitchForwardingInformationBaseMapThriftType =
    std::map<std::string, ForwardingInformationBaseMapThriftType>;

class MultiSwitchForwardingInformationBaseMap;

using MultiSwitchForwardingInformationBaseMapTraits =
    ThriftMultiSwitchMapNodeTraits<
        MultiSwitchForwardingInformationBaseMap,
        MultiSwitchForwardingInformationBaseMapTypeClass,
        MultiSwitchForwardingInformationBaseMapThriftType,
        ForwardingInformationBaseMap>;

class HwSwitchMatcher;

class MultiSwitchForwardingInformationBaseMap
    : public ThriftMultiSwitchMapNode<
          MultiSwitchForwardingInformationBaseMap,
          MultiSwitchForwardingInformationBaseMapTraits> {
 public:
  using Traits = MultiSwitchForwardingInformationBaseMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchForwardingInformationBaseMap,
      MultiSwitchForwardingInformationBaseMapTraits>;
  using BaseT::modify;

  MultiSwitchForwardingInformationBaseMap() {}
  virtual ~MultiSwitchForwardingInformationBaseMap() {}

  void updateForwardingInformationBaseContainer(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer,
      const HwSwitchMatcher& matcher);

  MultiSwitchForwardingInformationBaseMap* modify(
      std::shared_ptr<SwitchState>* state);

  std::pair<uint64_t, uint64_t> getRouteCount() const;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
