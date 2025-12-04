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
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class FibInfo;

USE_THRIFT_COW(FibInfo);

RESOLVE_STRUCT_MEMBER(
    FibInfo,
    switch_state_tags::fibsMap,
    ForwardingInformationBaseMap);

class FibInfo : public ThriftStructNode<FibInfo, state::FibInfoFields> {
 public:
  using Base = ThriftStructNode<FibInfo, state::FibInfoFields>;
  using Base::modify;

  FibInfo() = default;

  std::shared_ptr<ForwardingInformationBaseMap> getfibsMap() const {
    return safe_cref<switch_state_tags::fibsMap>();
  }

  FibInfo* modify(std::shared_ptr<SwitchState>* state);

  void updateFibContainer(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer,
      std::shared_ptr<SwitchState>* state);

  // Get FibContainer directly when VRF is specified
  std::shared_ptr<ForwardingInformationBaseContainer> getFibContainerIf(
      RouterID vrf) const {
    auto fibsMap = getfibsMap();
    if (!fibsMap) {
      return nullptr;
    }
    return fibsMap->getFibContainerIf(vrf);
  }

  // Get route count (v4, v6)
  std::pair<uint64_t, uint64_t> getRouteCount() const;

 private:
  // Inherit constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
