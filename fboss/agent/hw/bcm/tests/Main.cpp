/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Main.h"
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

DECLARE_bool(setup_for_warmboot);

using namespace facebook::fboss;

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  fbossInit(argc, argv);

  // Run the tests
  auto ret = RUN_ALL_TESTS();
  if (FLAGS_setup_for_warmboot) {
    // For warmboot we don't want destructors of static/global
    // vars to run so exit early.
    _exit(ret);
  }
  return ret;
}
