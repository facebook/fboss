/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/TestUtils.h"

#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NeighborResponseTable.h"
#include "fboss/agent/state/NdpEntry.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::MacAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

TEST(ArpEntry, serialize) {
  auto entry = std::make_unique<ArpEntry>(
    IPAddressV4("192.168.0.1"),
    MacAddress("01:01:01:01:01:01"),
    PortDescriptor(PortID(0)),
    InterfaceID(0)
  );

  auto serialized = entry->toFollyDynamic();
  auto entryBack = ArpEntry::fromFollyDynamic(serialized);

  // Any entry we deserialize should come back in the UNVERIFIED state
  EXPECT_EQ(entryBack->getState(), NeighborState::UNVERIFIED);

  // manually modify the state to the original state, so that we can compare
  // two entries using operator ==
  entryBack->setState(entry->getState());
  EXPECT_TRUE(*entry == *entryBack);
}

TEST(NdpEntry, serialize) {
  auto entry = std::make_unique<NdpEntry>(
    IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
    MacAddress("01:01:01:01:01:01"),
    PortDescriptor(PortID(0)),
    InterfaceID(0)
  );

  auto serialized = entry->toFollyDynamic();
  auto entryBack = NdpEntry::fromFollyDynamic(serialized);

  // Any entry we deserialize should come back in the UNVERIFIED state
  EXPECT_EQ(entryBack->getState(), NeighborState::UNVERIFIED);

  // manually modify the state to the original state, so that we can compare
  // two entries using operator ==
  entryBack->setState(entry->getState());
  EXPECT_TRUE(*entry == *entryBack);
}

TEST(NeighborResponseEntry, serialize) {
  auto entry = std::make_unique<NeighborResponseEntry>(
    MacAddress("01:01:01:01:01:01"),
    InterfaceID(0)
  );

  auto serialized = entry->toFollyDynamic();
  auto entryBack = NeighborResponseEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == entryBack);
}
