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

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <iostream>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::fboss::fbossInit(argc, argv);

  // Run the tests
  int ret = RUN_ALL_TESTS();
  // Use std::_Exit() to skip static destructors. initFacebook() spawns
  // background threads (async_trace, scribe) that may still be running;
  // normal exit() destroys globals (e.g. boost::regex mem_block_cache) while
  // those threads still reference them, causing heap-use-after-free under ASan.
  folly::LoggerDB::get().flushAllHandlers();
  std::cout.flush();
  std::cerr.flush();
  std::_Exit(ret);
}
