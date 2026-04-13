// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "common/init/light.h"
#include "fboss/agent/AgentFeatures.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  facebook::init::InitFacebookLight init(&argc, &argv);

  FLAGS_enable_nexthop_id_manager = true;

  return RUN_ALL_TESTS();
}
