/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/FdbApi.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(FdbEntryTest, serDeserv) {
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 1000, mac);
  EXPECT_EQ(f, SaiFdbTraits::FdbEntry::fromFollyDynamic(f.toFollyDynamic()));
}
