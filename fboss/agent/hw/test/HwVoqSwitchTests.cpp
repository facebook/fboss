// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwVoqSwitchTest : public HwTest {};

TEST_F(HwVoqSwitchTest, init) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

} // namespace facebook::fboss
