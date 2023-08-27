// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

using namespace ::testing;
using namespace facebook::fboss;

class EmptyHwTest : public HwTest {};

/*
 * This test is just to make sure the basic setup of any test works. If this
 * fails, any other tests will fail too. Thus the pass rate of this test can be
 * used as a baseline to compare the health of other tests against. Logically,
 * this test would try to refresh state machines till all the cabled
 * transceivers are programmed
 */
TEST_F(EmptyHwTest, CheckInit) {
  verifyAcrossWarmBoots([]() {}, []() {});
}
