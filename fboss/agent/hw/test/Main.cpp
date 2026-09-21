/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossInit.h"

#include <cstdlib>

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);

  if (!GTEST_FLAG_GET(list_tests)) {
    facebook::fboss::fbossInit(argc, argv);
  }

  int rc = RUN_ALL_TESTS();

  // Skip destructors during --gtest_list_tests to avoid an opt-asan UAF
  // race with initFacebook background fibers. Real test runs use the
  // normal return so ~SaiApiTable runs (Credo B52 warm boot). See
  // D105067110.
  if (GTEST_FLAG_GET(list_tests)) {
    std::_Exit(rc);
  }
  return rc;
}
