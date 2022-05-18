/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/weutil/hw_test/WeutilTest.h"
#include "fboss/platform/weutil/Weutil.h"

namespace facebook::fboss::platform {

WeutilTest::~WeutilTest() {}

void WeutilTest::SetUp() {
  weutilInstance = get_plat_weutil();
}

void WeutilTest::TearDown() {}

TEST_F(WeutilTest, getWedgeInfo) {
  EXPECT_GT(weutilInstance->getInfo().size(), 0);
}

} // namespace facebook::fboss::platform
