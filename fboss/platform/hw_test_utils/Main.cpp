/*
 *  Copyright (c) 2004-present, Facebook, Inc.
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
#ifndef IS_OSS
// Only include Meta specific init interface if it's Meta build, not OSS
#include "common/init/Init.h"
#endif // IS_OSS

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
#ifndef IS_OSS
  // Meta specific init routines are not executed in OSS environment
  facebook::initFacebook(&argc, &argv);
#endif // IS_OSS
  // Run the tests
  return RUN_ALL_TESTS();
}
