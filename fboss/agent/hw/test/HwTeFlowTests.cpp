/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"

namespace facebook::fboss {

class HwTeFlowTest : public HwTest {
 protected:
  void SetUp() override {
    FLAGS_enable_exact_match = true;
    HwTest::SetUp();
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::EXACT_MATCH);
  }
};

TEST_F(HwTeFlowTest, VerifyTeFlowGroupEnable) {
  if (this->skipTest()) {
    return;
  }

  auto verify = [&]() {
    EXPECT_TRUE(utility::validateTeFlowGroupEnabled(getHwSwitch()));
  };

  verify();
}

} // namespace facebook::fboss
