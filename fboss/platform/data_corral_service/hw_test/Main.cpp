/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/logging/Init.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/Init.h"

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(argc, argv);
  // Run the tests
  return RUN_ALL_TESTS();
}
