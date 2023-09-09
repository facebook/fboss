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
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

/*
 * The filename provided to logger uses the string "fboss/agent/.cpp".
 * LoggerDB recursively finds the parent by stripping the filename using
 * the separator and finally ends with "fboss" in the string and the
 * corresponding LogLevel is picked to print the log.
 *
 * In OSS, the file name provided to logger uses the string
 * "/var/FBOSS/tmp_build_dir/fboss-git/fboss/agent*.cpp". When LoggerDB
 * recursively finds the parent by stripping the filename using the separator,
 * it never finds "fboss" string in its DB and ends up setting the default
 * logging level which is INFO.
 *
 * The stripping is different internally and in OSS because when buck2 is used
 * to build, there is an additional processing done to the filenames which
 * removes few strings from the beginning of the path.
 *
 * To fix this issue, we are adding a flag for OSS builds to set the logging
 * without the category.
 */
#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");
#endif

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::fboss::fbossInit(argc, argv);

  // Run the tests
  return RUN_ALL_TESTS();
}
