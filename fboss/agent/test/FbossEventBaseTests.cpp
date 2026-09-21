// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/FbossEventBase.h"

namespace facebook::fboss {

TEST(FbossEventBaseTest, VerifyOperDelta) {
  FbossEventBase fevb{"test"};
  for (int i = 0; i < 2 * FLAGS_fboss_event_base_queue_limit; i++) {
    fevb.runInFbossEventBaseThread([&]() {});
  }
  EXPECT_EQ(
      fevb.getNotificationQueueSize(), FLAGS_fboss_event_base_queue_limit);
}
} // namespace facebook::fboss
