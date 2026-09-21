// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/lib/CommonThriftUtils.h"

using namespace ::testing;
using namespace facebook::fboss;
using namespace std::chrono_literals;

TEST(CommonThriftUtilsTest, ExpBackoffTest) {
  // basic test
  {
    auto backoff = ExpBackoff(1, 10, false /* use jitter*/);
    EXPECT_EQ(backoff.getCurTimeout(), 1ms);
    backoff.reportSuccess();
    EXPECT_EQ(backoff.getCurTimeout(), 1ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 2ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 4ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 8ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportSuccess();
    EXPECT_EQ(backoff.getCurTimeout(), 1ms);
  }
  // initial backoff == max backoff
  {
    auto backoff = ExpBackoff(10, 10, false /* use jitter*/);
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportSuccess();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportError();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
    backoff.reportSuccess();
    EXPECT_EQ(backoff.getCurTimeout(), 10ms);
  }
}
