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

#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseDelta.h"
#include "fboss/agent/state/MapDelta.h"
#include "fboss/agent/state/NodeMapDelta.h"

namespace facebook::fboss {

class FibInfoDelta : public DeltaValue<FibInfo> {
 public:
  using DeltaValue<FibInfo>::DeltaValue;

  ForwardingInformationBaseMapDelta getFibsMapDelta() const {
    auto oldMap = getOld() ? getOld()->getfibsMap().get() : nullptr;
    auto newMap = getNew() ? getNew()->getfibsMap().get() : nullptr;
    return ForwardingInformationBaseMapDelta(oldMap, newMap);
  }
};

template <typename IGNORED>
struct MultiSwitchFibInfoMapDeltaTraits {
  using mapped_type = typename MultiSwitchFibInfoMap::mapped_type;
  using Extractor = ExtractorT<MultiSwitchFibInfoMap>;
  using Delta = FibInfoDelta;
  using NodeWrapper = typename Delta::NodeWrapper;
  using DeltaValueIterator =
      DeltaValueIteratorT<MultiSwitchFibInfoMap, Delta, Extractor>;
  using MapPointerTraits = MapPointerTraitsT<MultiSwitchFibInfoMap>;
};

using MultiSwitchFibInfoMapDelta =
    MapDelta<MultiSwitchFibInfoMap, MultiSwitchFibInfoMapDeltaTraits>;

} // namespace facebook::fboss
