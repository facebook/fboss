/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include "folly/init/Init.h"

int main(int argc, char** argv) {
  // Set timezone to PST to match test expectations
  // Tests expect timestamps to be formatted in Pacific timezone
  setenv("TZ", "PST8PDT", 1);
  tzset();

  testing::InitGoogleTest(&argc, argv);
  const folly::Init init(&argc, &argv);
  return RUN_ALL_TESTS();
}
