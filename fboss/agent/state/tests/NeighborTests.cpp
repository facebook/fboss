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
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/NeighborResponseTable.h"

#include <gtest/gtest.h>

namespace {
auto constexpr kState = "state";
}

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

template <typename NeighborEntryT>
void serializeTest(const NeighborEntryT& entry) {
  auto serialized = entry.toFollyDynamic();
  auto entryBack = NeighborEntryT::fromFollyDynamic(serialized);

  EXPECT_EQ(entry, *entryBack);
}

TEST(ArpEntry, serialize) {
  auto entry = std::make_unique<ArpEntry>(
      IPAddressV4("192.168.0.1"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(1)),
      InterfaceID(10));
  serializeTest(*entry);
}

TEST(NdpEntry, serialize) {
  auto entry = std::make_unique<NdpEntry>(
      IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(10)),
      InterfaceID(10));
  serializeTest(*entry);
}

template <typename NeighborEntryT>
void serializeMissingStateTest(
    const std::vector<std::unique_ptr<NeighborEntryT>>& entries,
    const std::vector<NeighborState>& expectedStates) {
  for (auto i = 0; i < entries.size(); ++i) {
    auto serialized = entries[i]->toFollyDynamic();
    serialized.erase(kState);
    auto entryBack = NeighborEntryT::fromFollyDynamic(serialized);
    EXPECT_EQ(entryBack->getState(), expectedStates[i]);
  }
}

TEST(ArpEntry, serializeMissingState) {
  std::vector<std::unique_ptr<ArpEntry>> entries;
  entries.emplace_back(std::make_unique<ArpEntry>(
      IPAddressV4("192.168.0.1"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(1)),
      InterfaceID(10)));
  entries.emplace_back(std::make_unique<ArpEntry>(
      IPAddressV4("192.168.0.1"),
      MacAddress("ff:ff:ff:ff:ff:ff"),
      PortDescriptor(PortID(0)),
      InterfaceID(0)));

  std::vector<NeighborState> expectedStates = {NeighborState::UNVERIFIED,
                                               NeighborState::PENDING};
  serializeMissingStateTest(entries, expectedStates);
}

TEST(NdpEntry, serializeMissingState) {
  std::vector<std::unique_ptr<NdpEntry>> entries;
  entries.emplace_back(std::make_unique<NdpEntry>(
      IPAddressV6("2401:db00:21:70cb:face:0:96:1"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(10)),
      InterfaceID(10)));
  entries.emplace_back(std::make_unique<NdpEntry>(
      IPAddressV6("2401:db00:21:70cb:face:0:96:2"),
      MacAddress("ff:ff:ff:ff:ff:ff"),
      PortDescriptor(PortID(0)),
      InterfaceID(0)));
  std::vector<NeighborState> expectedStates = {NeighborState::UNVERIFIED,
                                               NeighborState::PENDING};
  serializeMissingStateTest(entries, expectedStates);
}

TEST(NeighborResponseEntry, serialize) {
  auto entry = std::make_unique<NeighborResponseEntry>(
      MacAddress("01:01:01:01:01:01"), InterfaceID(0));

  auto serialized = entry->toFollyDynamic();
  auto entryBack = NeighborResponseEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == entryBack);
}
