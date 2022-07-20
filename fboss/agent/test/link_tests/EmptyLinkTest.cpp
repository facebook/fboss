// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/test/link_tests/LinkTest.h"

using namespace ::testing;
using namespace facebook::fboss;

class EmptyLinkTest : public LinkTest {};

TEST_F(EmptyLinkTest, CheckInit) {
  verifyAcrossWarmBoots([]() {}, []() {});
}
