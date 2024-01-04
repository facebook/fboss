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

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

class ResourceAccountant {
 public:
  explicit ResourceAccountant(const HwAsicTable* asicTable);

 private:
  int getMemberCountForEcmpGroup(const RouteNextHopEntry& fwd) const;

  const HwAsicTable* asicTable_;
  bool nativeWeightedEcmp_{true};

  FRIEND_TEST(ResourceAccountantTest, getMemberCountForEcmpGroup);
};
} // namespace facebook::fboss
