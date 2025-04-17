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
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <memory>

namespace facebook::fboss {
class StateDelta;
class SwitchState;
class EcmpGroupConsolidator {
 public:
  using NextHopSet = RouteNextHopEntry::NextHopSet;
  using NextHopGroupId = uint32_t;
  std::shared_ptr<SwitchState> consolidate(const StateDelta& delta);

 private:
  static uint32_t constexpr kMinNextHopGroupId = 1;
  NextHopGroupId findNextAvailableId() const;
  std::unordered_map<NextHopSet, NextHopGroupId> nextHopGroup2Id_;
};
} // namespace facebook::fboss
