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

using MultiForwardingInformationBaseMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, ForwardingInformationBaseMapClass>;
using MultiForwardingInformationBaseMapThriftType =
    std::map<std::string, ForwardingInformationBaseMapThriftType>;

class MultiForwardingInformationBaseMap;

using MultiForwardingInformationBaseMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiForwardingInformationBaseMap,
    MultiForwardingInformationBaseMapTypeClass,
    MultiForwardingInformationBaseMapThriftType,
    ForwardingInformationBaseMap>;

class HwSwitchMatcher;

class MultiForwardingInformationBaseMap
    : public ThriftMultiSwitchMapNode<
          MultiForwardingInformationBaseMap,
          MultiForwardingInformationBaseMapTraits> {
 public:
  using Traits = MultiForwardingInformationBaseMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiForwardingInformationBaseMap,
      MultiForwardingInformationBaseMapTraits>;
  using BaseT::modify;

  MultiForwardingInformationBaseMap() {}
  virtual ~MultiForwardingInformationBaseMap() {}

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
