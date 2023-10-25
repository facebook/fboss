// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/AgentDirectoryUtil.h"

using namespace ::testing;

using namespace facebook::fboss;

TEST(AgentDirectoryUtilTests, VerifyColdBootFiles) {
  AgentDirectoryUtil util;
  EXPECT_EQ(
      util.getSwColdBootOnceFile(),
      "/dev/shm/fboss/warm_boot/sw_cold_boot_once");
  EXPECT_EQ(
      util.getHwColdBootOnceFile(0),
      "/dev/shm/fboss/warm_boot/hw_cold_boot_once_0");
  EXPECT_EQ(
      util.getHwColdBootOnceFile(1),
      "/dev/shm/fboss/warm_boot/hw_cold_boot_once_1");
  EXPECT_EQ(
      util.getColdBootOnceFile(), "/dev/shm/fboss/warm_boot/cold_boot_once_0");
}
