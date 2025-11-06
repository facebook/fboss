/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
namespace {
const auto kTestMac1 = folly::MacAddress("01:02:03:04:05:06");
const auto kTestMac2 = folly::MacAddress("01:02:03:04:05:07");
} // namespace

TEST(MacTableTest, thriftyConversion) {
  MacTable table;
  table.addEntry(
      std::make_shared<MacEntry>(
          kTestMac1,
          PortDescriptor(PortID(1)),
          std::optional<cfg::AclLookupClass>(
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0)));
  table.addEntry(
      std::make_shared<MacEntry>(
          kTestMac2,
          PortDescriptor(PortID(2)),
          std::optional<cfg::AclLookupClass>(
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
          MacEntryType::STATIC_ENTRY));

  auto tableSptr = std::make_shared<MacTable>();
  tableSptr->fromThrift(table.toThrift());
  EXPECT_EQ(tableSptr->toThrift(), table.toThrift());
}
