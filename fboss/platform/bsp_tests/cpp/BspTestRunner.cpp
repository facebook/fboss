// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include "fboss/platform/bsp_tests/cpp/BspTestEnvironment.h"

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;
using namespace facebook::fboss::platform::bsp_tests::cpp;

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  helpers::init(&argc, &argv);

  auto platformName =
      helpers::PlatformNameLib().getPlatformName().value_or("unknown");

  XLOG(INFO) << "Starting BSP tests";
  XLOG(INFO) << "Platform: " << platformName;

  BspTestEnvironment::GetInstance(platformName);

  return RUN_ALL_TESTS();
}
