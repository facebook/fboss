/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/G202xAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/hw/switch_asics/YubaAsic.h"

#include <gtest/gtest.h>
#include <memory>

using namespace facebook::fboss;

namespace {
cfg::SwitchInfo npuSwitchInfo() {
  cfg::SwitchInfo switchInfo;
  switchInfo.switchType() = cfg::SwitchType::NPU;
  switchInfo.switchMac() = "02:00:00:00:0F:0B";
  switchInfo.switchIndex() = 0;
  return switchInfo;
}
} // namespace

// NEXT_HOP_GROUP_MEMBER_MONITORED_OBJECT says the ASIC cannot infer a
// protection group PRIMARY member's monitored object from that member's next
// hop, so FBOSS has to program it. Broadcom infers it, so programming it there
// would be a behavior change to the existing IP FRR path -- this test pins down
// which ASICs are opted in.
TEST(MonitoredObjectFeatureTest, onlyYubaNeedsMonitoredObjectProgrammed) {
  YubaAsic yuba{0, npuSwitchInfo()};
  EXPECT_TRUE(yuba.isSupported(
      HwAsic::Feature::NEXT_HOP_GROUP_MEMBER_MONITORED_OBJECT));

  // Broadcom infers the monitored object.
  Tomahawk5Asic tomahawk5{0, npuSwitchInfo()};
  EXPECT_FALSE(tomahawk5.isSupported(
      HwAsic::Feature::NEXT_HOP_GROUP_MEMBER_MONITORED_OBJECT));

  // Another Leaba ASIC, deliberately NOT opted in yet: SRv6 midpoint FRR is
  // only being brought up on G200.
  G202xAsic g202x{0, npuSwitchInfo()};
  EXPECT_FALSE(g202x.isSupported(
      HwAsic::Feature::NEXT_HOP_GROUP_MEMBER_MONITORED_OBJECT));
}
