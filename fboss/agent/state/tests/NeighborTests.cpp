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
#include "fboss/agent/state/Vlan.h"

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
  validateThriftyMigration(*entry);
  serializeTest(*entry);
}

TEST(NdpEntry, serialize) {
  auto entry = std::make_unique<NdpEntry>(
      IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(10)),
      InterfaceID(10));
  validateThriftyMigration(*entry);
  serializeTest(*entry);
}

TEST(ArpTable, serialize) {
  ArpTable table;
  table.addEntry(
      IPAddressV4("192.168.0.1"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(10)),
      InterfaceID(10),
      NeighborState::REACHABLE);
  table.addEntry(
      IPAddressV4("192.168.0.2"),
      MacAddress("01:01:01:01:01:02"),
      PortDescriptor(PortID(11)),
      InterfaceID(11),
      NeighborState::PENDING);

  validateThriftyMigration(table);
  serializeTest(table);
}

TEST(NdpTable, serialize) {
  NdpTable table;
  table.addEntry(
      IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
      MacAddress("01:01:01:01:01:01"),
      PortDescriptor(PortID(10)),
      InterfaceID(10),
      NeighborState::REACHABLE);
  table.addEntry(
      IPAddressV6("2401:db00:21:70cb:face:0:96:1"),
      MacAddress("01:01:01:01:01:02"),
      PortDescriptor(PortID(11)),
      InterfaceID(11),
      NeighborState::PENDING);

  validateThriftyMigration(table);
  serializeTest(table);
}

TEST(NeighborResponseEntry, serialize) {
  auto entry = std::make_unique<NeighborResponseEntry>(
      MacAddress("01:01:01:01:01:01"), InterfaceID(0));

  auto serialized = entry->toFollyDynamic();
  auto entryBack = NeighborResponseEntry::fromFollyDynamic(serialized);

  EXPECT_TRUE(*entry == entryBack);
}

TEST(NeighborResponseTableTest, modify) {
  auto ip1 = IPAddressV4("192.168.0.1"), ip2 = IPAddressV4("192.168.0.2");
  auto mac1 = MacAddress("01:01:01:01:01:01"),
       mac2 = MacAddress("01:01:01:01:01:02");

  auto state = std::make_shared<SwitchState>();
  auto vlan = std::make_shared<Vlan>(VlanID(2001), "vlan1");
  auto arpResponseTable = std::make_shared<ArpResponseTable>();
  arpResponseTable->setEntry(ip1, mac1, InterfaceID(0));
  vlan->setArpResponseTable(arpResponseTable);
  state->getVlans()->addVlan(vlan);

  // modify unpublished state
  EXPECT_EQ(vlan.get(), vlan->modify(&state));

  arpResponseTable = std::make_shared<ArpResponseTable>();
  arpResponseTable->setEntry(ip2, mac2, InterfaceID(1));
  vlan->setArpResponseTable(arpResponseTable);

  // modify unpublished state
  EXPECT_EQ(vlan.get(), vlan->modify(&state));

  vlan->publish();
  auto modifiedVlan = vlan->modify(&state);

  arpResponseTable = std::make_shared<ArpResponseTable>();
  arpResponseTable->setEntry(ip1, mac1, InterfaceID(0));
  EXPECT_DEATH(vlan->setArpResponseTable(arpResponseTable), "!isPublished");
  modifiedVlan->setArpResponseTable(arpResponseTable);

  EXPECT_NE(vlan.get(), modifiedVlan);

  EXPECT_FALSE(vlan->getArpResponseTable()->getEntry(ip1).has_value());
  EXPECT_TRUE(vlan->getArpResponseTable()->getEntry(ip2).has_value());
  EXPECT_TRUE(modifiedVlan->getArpResponseTable()->getEntry(ip1).has_value());
  EXPECT_FALSE(modifiedVlan->getArpResponseTable()->getEntry(ip2).has_value());
}

TEST(ArpResponseTableTest, nodeMapForwardConversion) {
  auto ip1 = IPAddressV4("192.168.0.1"), ip2 = IPAddressV4("192.168.0.2");
  auto mac1 = MacAddress("01:01:01:01:01:01"),
       mac2 = MacAddress("01:01:01:01:01:02");
  auto intf1 = InterfaceID(10), intf2 = InterfaceID(11);

  auto arpResponseTable = std::make_shared<ArpResponseTable>();
  arpResponseTable->setEntry(ip1, mac1, intf1);
  arpResponseTable->setEntry(ip2, mac2, intf2);

  auto serialized = arpResponseTable->toFollyDynamic();
  auto deserialized = ArpResponseTableThrifty::fromFollyDynamic(serialized);

  EXPECT_EQ(deserialized->size(), 2);
  auto entry1 = deserialized->getNode(ip1);
  EXPECT_NE(entry1, nullptr);
  EXPECT_EQ(entry1->getIP(), ip1);
  EXPECT_EQ(entry1->getMac(), mac1);
  EXPECT_EQ(entry1->getInterfaceID(), intf1);
  auto entry2 = deserialized->getNode(ip2);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->getIP(), ip2);
  EXPECT_EQ(entry2->getMac(), mac2);
  EXPECT_EQ(entry2->getInterfaceID(), intf2);
}

TEST(ArpResponseTableTest, nodeMapBackwardsConversion) {
  auto ip1 = IPAddressV4("192.168.0.1"), ip2 = IPAddressV4("192.168.0.2");
  auto mac1 = MacAddress("01:01:01:01:01:01"),
       mac2 = MacAddress("01:01:01:01:01:02");
  auto intf1 = InterfaceID(10), intf2 = InterfaceID(11);

  auto arpResponseTable = std::make_shared<ArpResponseTableThrifty>();
  arpResponseTable->addNode(
      std::make_shared<ArpResponseEntryThrifty>(ip1, mac1, intf1));
  arpResponseTable->addNode(
      std::make_shared<ArpResponseEntryThrifty>(ip2, mac2, intf2));

  auto serialized = arpResponseTable->toFollyDynamic();
  auto deserialized = ArpResponseTable::fromFollyDynamic(serialized);

  EXPECT_EQ(deserialized->getTable().size(), 2);
  auto entry1 = deserialized->getEntry(ip1);
  EXPECT_TRUE(entry1.has_value());
  EXPECT_EQ(entry1->mac, mac1);
  EXPECT_EQ(entry1->interfaceID, intf1);
  auto entry2 = deserialized->getEntry(ip2);
  EXPECT_TRUE(entry2.has_value());
  EXPECT_EQ(entry2->mac, mac2);
  EXPECT_EQ(entry2->interfaceID, intf2);
}

TEST(NdpResponseTableTest, nodeMapForwardConversion) {
  auto ip1 = IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
       ip2 = IPAddressV6("2401:db00:21:70cb:face:0:96:1");
  auto mac1 = MacAddress("01:01:01:01:01:01"),
       mac2 = MacAddress("01:01:01:01:01:02");
  auto intf1 = InterfaceID(10), intf2 = InterfaceID(11);

  auto ndpResponseTable = std::make_shared<NdpResponseTable>();
  ndpResponseTable->setEntry(ip1, mac1, intf1);
  ndpResponseTable->setEntry(ip2, mac2, intf2);

  auto serialized = ndpResponseTable->toFollyDynamic();
  auto deserialized = NdpResponseTableThrifty::fromFollyDynamic(serialized);

  EXPECT_EQ(deserialized->size(), 2);
  auto entry1 = deserialized->getNode(ip1);
  EXPECT_NE(entry1, nullptr);
  EXPECT_EQ(entry1->getIP(), ip1);
  EXPECT_EQ(entry1->getMac(), mac1);
  EXPECT_EQ(entry1->getInterfaceID(), intf1);
  auto entry2 = deserialized->getNode(ip2);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->getIP(), ip2);
  EXPECT_EQ(entry2->getMac(), mac2);
  EXPECT_EQ(entry2->getInterfaceID(), intf2);
}

TEST(NdpResponseTableTest, nodeMapBackwardsConversion) {
  auto ip1 = IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
       ip2 = IPAddressV6("2401:db00:21:70cb:face:0:96:1");
  auto mac1 = MacAddress("01:01:01:01:01:01"),
       mac2 = MacAddress("01:01:01:01:01:02");
  auto intf1 = InterfaceID(10), intf2 = InterfaceID(11);

  auto ndpResponseTable = std::make_shared<NdpResponseTableThrifty>();
  ndpResponseTable->addNode(
      std::make_shared<NdpResponseEntryThrifty>(ip1, mac1, intf1));
  ndpResponseTable->addNode(
      std::make_shared<NdpResponseEntryThrifty>(ip2, mac2, intf2));

  auto serialized = ndpResponseTable->toFollyDynamic();
  auto deserialized = NdpResponseTable::fromFollyDynamic(serialized);

  EXPECT_EQ(deserialized->getTable().size(), 2);
  auto entry1 = deserialized->getEntry(ip1);
  EXPECT_TRUE(entry1.has_value());
  EXPECT_EQ(entry1->mac, mac1);
  EXPECT_EQ(entry1->interfaceID, intf1);
  auto entry2 = deserialized->getEntry(ip2);
  EXPECT_TRUE(entry2.has_value());
  EXPECT_EQ(entry2->mac, mac2);
  EXPECT_EQ(entry2->interfaceID, intf2);
}
