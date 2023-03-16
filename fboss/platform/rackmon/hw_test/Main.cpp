/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/rackmon/hw_test/RackmonHwTest.h"

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging("rackmon_hw_test");
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(argc, argv);
  FLAGS_minloglevel = 4;
  rackmonsvc::RackmonThriftService service;
  // Run the tests
  return RUN_ALL_TESTS();
}
