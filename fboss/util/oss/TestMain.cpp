// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

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
