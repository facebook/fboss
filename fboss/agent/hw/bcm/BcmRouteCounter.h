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

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include <folly/dynamic.h>
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/lib/RefMap.h"

#define STAT_MODEID_INVALID -1

namespace facebook::fboss {

using BcmRouteCounterID = uint32_t;

/**
 * BcmRouteCounter represents counter associated with L3 route object.
 */
class BcmRouteCounter {
 public:
  BcmRouteCounter(BcmSwitch* hw, RouteCounterID id, int modeId);
  ~BcmRouteCounter();
  BcmRouteCounterID getHwCounterID() const {
    return hwCounterId_;
  }

 private:
  // no copy or assign
  BcmRouteCounter(const BcmRouteCounter&) = delete;
  BcmRouteCounter& operator=(const BcmRouteCounter&) = delete;
  BcmSwitch* hw_;
  BcmRouteCounterID hwCounterId_;
};

class BcmRouteCounterTable {
 public:
  explicit BcmRouteCounterTable(BcmSwitch* hw);
  ~BcmRouteCounterTable();
  void setMaxRouteCounterIDs(uint32_t count) {
    maxRouteCounterIDs_ = count;
  }
  std::shared_ptr<BcmRouteCounter> referenceOrEmplaceCounterID(
      RouteCounterID id);
  // used for testing purpose
  std::optional<BcmRouteCounterID> getHwCounterID(
      std::optional<RouteCounterID> counterID) const;
  folly::dynamic toFollyDynamic() const;

  static constexpr folly::StringPiece kRouteCounters{"routeCounters"};
  static constexpr folly::StringPiece kRouteCounterIDs{"routeCounterIDs"};
  static constexpr folly::StringPiece kGlobalModeId{"globalModeId"};

 private:
  uint32_t createStatGroupModeId();
  BcmSwitch* hw_;
  FlatRefMap<RouteCounterID, BcmRouteCounter> counterIDs_;
  int globalIngressModeId_{STAT_MODEID_INVALID};
  uint32_t maxRouteCounterIDs_{0};
};
} // namespace facebook::fboss
