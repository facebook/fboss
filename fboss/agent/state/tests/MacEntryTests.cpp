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

#include <gtest/gtest.h>

using namespace facebook::fboss;
namespace {
const auto kMac = folly::MacAddress("01:02:03:04:05:06");
}

TEST(MacEntryTest, toFromFollyDynamic) {
  MacEntry entryDynamic(
      kMac,
      PortDescriptor(PortID(1)),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  EXPECT_EQ(
      *MacEntry::fromFollyDynamic(entryDynamic.toFollyDynamic()), entryDynamic);
  MacEntry entryStatic(
      kMac,
      PortDescriptor(PortID(1)),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  EXPECT_EQ(
      *MacEntry::fromFollyDynamic(entryStatic.toFollyDynamic()), entryStatic);
}

TEST(MacEntryTest, Compare) {
  MacEntry entryDynamic(
      kMac,
      PortDescriptor(PortID(1)),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  MacEntry entryStatic(
      kMac,
      PortDescriptor(PortID(1)),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  EXPECT_NE(entryStatic, entryDynamic);
}

TEST(MacEntry, fromJSON) {
  std::string jsonStrMissingEntryType = R"(
  {
      "mac": "01:02:03:04:05:06",
      "portId": 1
  })";
  auto entry = MacEntry::fromJson(jsonStrMissingEntryType);
  EXPECT_EQ(
      *entry,
      MacEntry(
          kMac,
          PortDescriptor(PortID(1)),
          std::nullopt,
          MacEntryType::DYNAMIC_ENTRY));
}

TEST(MacEntry, fromJSONWithType) {
  std::string jsonStrMissingEntryType = R"(
  {
      "mac": "01:02:03:04:05:06",
      "portId": {
            "portId": 1,
            "portType": 0
      }
  })";
  auto entry = MacEntry::fromJson(jsonStrMissingEntryType);
  EXPECT_EQ(
      *entry,
      MacEntry(
          kMac,
          PortDescriptor(PortID(1)),
          std::nullopt,
          MacEntryType::DYNAMIC_ENTRY));
}
