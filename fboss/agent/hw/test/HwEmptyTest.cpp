// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwEmptyTest : public HwTest {};

TEST_F(HwEmptyTest, CheckInit) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

} // namespace facebook::fboss
