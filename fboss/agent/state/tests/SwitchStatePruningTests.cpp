/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <map>
#include <set>

using namespace facebook::fboss;
using facebook::fboss::cfg::SwitchConfig;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::make_tuple;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::tuple;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

#define PP(node) NN(folly::toPrettyJson(node->toFollyDynamic()))

template <typename NTableT>
void checkChangedNeighborEntries(
    const shared_ptr<SwitchState>& state,
    const shared_ptr<SwitchState>& prunedState,
    const std::set<tuple<
        VlanID,
        typename NTableT::Entry::AddressType,
        shared_ptr<typename NTableT::Entry>>>& changedIPs,
    const std::set<pair<VlanID, typename NTableT::Entry::AddressType>>&
        addedIPs,
    const std::set<pair<VlanID, typename NTableT::Entry::AddressType>>&
        removedIPs) {
  using EntryT = typename NTableT::Entry;
  using AddressT = typename EntryT::AddressType;
  CHECK(addedIPs.empty());
  // We never add things to state when we prune a state
  StateDelta delta(state, prunedState);
  std::set<tuple<VlanID, AddressT, shared_ptr<EntryT>>> changedIPsFound;
  std::set<pair<VlanID, AddressT>> addedIPsFound;
  std::set<pair<VlanID, AddressT>> removedIPsFound;
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    for (const auto& entry : vlanDelta.template getNeighborDelta<NTableT>()) {
      auto entryBeforePrune = entry.getOld();
      auto entryAfterPrune = entry.getNew();
      CHECK(entryBeforePrune || entryAfterPrune);
      if (entryBeforePrune && entryAfterPrune) {
        // changed
        CHECK_EQ(entryBeforePrune->getIP(), entryAfterPrune->getIP());
        changedIPsFound.insert(make_tuple(
            static_cast<VlanID>(entryAfterPrune->getIntfID()),
            entryAfterPrune->getIP(),
            entryAfterPrune));
      } else if (entryBeforePrune) {
        // removed
        removedIPsFound.insert(make_pair(
            static_cast<VlanID>(entryBeforePrune->getIntfID()),
            entryBeforePrune->getIP()));
      } else {
        // added
        addedIPsFound.insert(make_pair(
            static_cast<VlanID>(entryAfterPrune->getIntfID()),
            entryAfterPrune->getIP()));
      }
    }
  }
  CHECK(changedIPs == changedIPsFound);
  CHECK(addedIPs == addedIPsFound);
  CHECK(removedIPs == removedIPsFound);
}

void registerPortsAndPopulateConfig(
    std::map<PortID, VlanID> port2Vlan,
    std::map<VlanID, MacAddress>* vlan2MacPtr,
    shared_ptr<SwitchState> state,
    SwitchConfig& config) {
  // Register ports and count Vlans
  set<VlanID> vlans;
  for (const auto& portAndVlan : port2Vlan) {
    auto port = portAndVlan.first;
    auto vlan = portAndVlan.second;
    registerPort(state, port, folly::to<string>("port", int(port)), scope());
    vlans.insert(vlan);
  }
  config.ports()->resize(port2Vlan.size());
  int index = 0;
  for (const auto& portAndVlan : port2Vlan) {
    preparedMockPortConfig(config.ports()[index++], int(portAndVlan.first));
  }

  index = 0;
  config.vlans()->resize(vlans.size());
  for (const auto& vlanTyped : vlans) {
    auto& vlanObj = config.vlans()[index++];
    int vlanInt = int(vlanTyped);
    *vlanObj.id() = vlanInt;
    *vlanObj.name() = folly::to<std::string>("vlan", vlanInt);
    vlanObj.intfID() = vlanInt; // interface and vlan ids are the same
  }

  config.vlanPorts()->resize(port2Vlan.size());
  index = 0;
  for (const auto& portAndVlan : port2Vlan) {
    auto& vlanPortObj = config.vlanPorts()[index++];
    int portInt = int(portAndVlan.first);
    int vlanInt = int(portAndVlan.second);
    *vlanPortObj.logicalPort() = portInt;
    *vlanPortObj.vlanID() = vlanInt;
    *vlanPortObj.emitTags() = false;
  }

  config.interfaces()->resize(vlans.size());
  index = 0;
  for (const auto& vlanTyped : vlans) {
    cfg::Interface& interfaceObj = config.interfaces()[index++];
    int vlanInt = int(vlanTyped);
    *interfaceObj.intfID() = vlanInt;
    *interfaceObj.vlanID() = vlanInt;
    if (vlan2MacPtr) {
      // If the map is provided, it should have outgoing mac for each
      // interface!
      auto macStr = (*vlan2MacPtr)[vlanTyped].toString();
      interfaceObj.mac() = macStr;
    }
  }
  return;
}

// Add NeighborEntry to given state. The added entry can be pending or resolved
// based on port value (nullptr indicates pending entry).
template <typename NTable>
void addNeighborEntry(
    shared_ptr<SwitchState>* state, // published or unpublished
    typename NTable::AddressType* ip,
    MacAddress* mac,
    VlanID* vlan,
    PortDescriptor* port) { // null port indicates pending entry
  CHECK(ip);
  CHECK(vlan);
  shared_ptr<NTable> newNeighborTable = make_shared<NTable>();
  NTable* newNeighborTablePtr = newNeighborTable.get();
  auto oldNeighborTable =
      (*state)->getVlans()->getNode(*vlan)->template getNeighborTable<NTable>();
  if (oldNeighborTable) {
    // This call changes state too
    newNeighborTablePtr = oldNeighborTable->modify(*vlan, state);
  }

  // Add entry to the arp table
  if (port) {
    newNeighborTablePtr->addEntry(
        *ip, *mac, *port, static_cast<InterfaceID>(*vlan));
  } else {
    newNeighborTablePtr->addPendingEntry(*ip, static_cast<InterfaceID>(*vlan));
  }

  if (!oldNeighborTable) {
    // There is no old arp table
    auto vlanPtr = (*state)->getVlans()->getNode(*vlan).get();
    vlanPtr = vlanPtr->modify(state, scope());
    vlanPtr->setNeighborTable(std::move(newNeighborTable));
  }
}

// Test that we can add Arp and Ndp entries to a state and revert them from the
// published state.
TEST(SwitchStatePruningTests, AddNeighborEntry) {
  auto platform = createMockPlatform();
  SwitchConfig config;
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();
  // state0 = empty state
  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)},
      {PortID(4), VlanID(22)},
      {PortID(5), VlanID(22)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")},
      {VlanID(22), MacAddress("fa:ce:b0:0c:22:00")}};

  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);
  // state0
  // ... register some ports, vlans, interfaces
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  ASSERT_NE(state1, nullptr);
  ASSERT_TRUE(!state1->isPublished());
  state1->publish();

  shared_ptr<SwitchState> state2{state1};
  // state1
  // ... add some arp entries to vlan 21 (by resetting arp table)
  //      - host1 (resolved entry)
  //      - host2 (resolved entry, IPv6)
  // state2

  auto host1ip = IPAddressV4("10.0.21.1");
  auto host1mac = MacAddress("fa:ce:b0:0c:21:01");
  auto host1vlan = VlanID(21);
  auto host1port = PortDescriptor(PortID(1));
  // Add host1 (resolved) for vlan21
  addNeighborEntry<ArpTable>(
      &state2, &host1ip, &host1mac, &host1vlan, &host1port);

  auto host2ip = IPAddressV6("face:b00c:0:21::2");
  auto host2mac = MacAddress("fa:ce:b0:0c:21:02");
  auto host2vlan = VlanID(21);
  auto host2port = PortDescriptor(PortID(1));
  auto host2intf = InterfaceID(21);

  addNeighborEntry<NdpTable>(
      &state2, &host2ip, &host2mac, &host2vlan, &host2port);

  state2->publish();

  shared_ptr<SwitchState> state3{state2};
  // state2
  //  ... add host3 to vlan21's arp table
  // state3
  // Add a new entry

  auto host3ip = IPAddressV4("10.0.21.3");
  auto host3mac = MacAddress("fa:ce:b0:0c:21:03");
  auto host3vlan = VlanID(21);
  auto host3port = PortDescriptor(PortID(2));
  addNeighborEntry<ArpTable>(
      &state3, &host3ip, &host3mac, &host3vlan, &host3port);
  state3->publish();

  auto state4 = state3;
  // state3
  //  ... prune host3 from vlan21
  // state4

  set<pair<VlanID, IPAddressV4>> removedV4;
  // Remove host3 from the arp table
  auto h3entry =
      state3->getVlans()->getNode(host3vlan)->getArpTable()->getEntry(host3ip);

  SwitchState::revertNewNeighborEntry<ArpEntry, ArpTable>(
      h3entry, nullptr, &state4);
  removedV4.insert(make_pair(host3vlan, host3ip));
  checkChangedNeighborEntries<ArpTable>(
      state3, state4, {} /* no changed */, {} /* no added */, removedV4);
  state4->publish();

  auto state5 = state4;
  // state4
  //  ... prune host2 from vlan21 (ipv6 example)
  // state5
  set<pair<VlanID, IPAddressV6>> removedV6;
  auto v6entry =
      state4->getVlans()->getNode(host2vlan)->getNdpTable()->getEntry(host2ip);
  SwitchState::revertNewNeighborEntry<NdpEntry, NdpTable>(
      v6entry, nullptr, &state5);
  removedV6.insert(make_pair(host2vlan, host2ip));
  checkChangedNeighborEntries<NdpTable>(state4, state5, {}, {}, removedV6);

  state5->publish();
}

// Test that we can update pending entries to resolved ones, and revert them
// back to pending.
TEST(SwitchStatePruningTests, ChangeNeighborEntry) {
  auto platform = createMockPlatform();
  SwitchConfig config;
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();
  // state0 -> empty state
  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)},
      {PortID(4), VlanID(22)},
      {PortID(5), VlanID(22)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")},
      {VlanID(22), MacAddress("fa:ce:b0:0c:22:00")}};

  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);
  // state0
  // ... register some ports, vlans, interfaces
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  ASSERT_NE(state1, nullptr);
  ASSERT_TRUE(!state1->isPublished());
  state1->publish();

  shared_ptr<SwitchState> state2{state1};
  // state1
  //  ... Add host1 to vlan21 (pending)
  //  ... Add host2 to vlan21 (pending)
  //  ... Add host3 (v6) to vlan22 (pending)
  // state2

  auto host1ip = IPAddressV4("10.0.21.1");
  auto host1vlan = VlanID(21);
  addNeighborEntry<ArpTable>(&state2, &host1ip, nullptr, &host1vlan, nullptr);

  auto host2ip = IPAddressV4("10.0.21.2");
  auto host2vlan = VlanID(21);
  addNeighborEntry<ArpTable>(&state2, &host2ip, nullptr, &host2vlan, nullptr);

  auto host3ip = IPAddressV6("face:b00c:0:22::3");
  auto host3vlan = VlanID(22);
  addNeighborEntry<NdpTable>(&state2, &host3ip, nullptr, &host3vlan, nullptr);

  state2->publish();

  auto entry1old =
      state2->getVlans()->getNode(host1vlan)->getArpTable()->getEntry(host1ip);
  auto entry2old =
      state2->getVlans()->getNode(host2vlan)->getArpTable()->getEntry(host2ip);
  auto entry3old =
      state2->getVlans()->getNode(host3vlan)->getNdpTable()->getEntry(host3ip);

  auto state3 = state2;
  // state2
  //  ... resolve arp entry for host2
  //  ... resolve ndp entry for host3
  // state3

  auto host2mac = MacAddress("fa:ce:b0:0c:21:02");
  auto host2port = PortDescriptor(PortID(2));
  auto host2interface = InterfaceID(21);

  auto arpTable21 =
      state3->getVlans()->getNode(host2vlan)->getArpTable()->modify(
          host2vlan, &state3);
  arpTable21->updateEntry(
      host2ip, host2mac, host2port, host2interface, NeighborState::REACHABLE);

  auto host3mac = MacAddress("fa:ce:b0:0c:22:03");
  auto host3port = PortDescriptor(PortID(4));
  auto host3interface = InterfaceID(22);

  auto ndpTable22 =
      state3->getVlans()->getNode(host3vlan)->getNdpTable()->modify(
          host3vlan, &state3);
  ndpTable22->updateEntry(
      host3ip, host3mac, host3port, host3interface, NeighborState::REACHABLE);

  state3->publish();

  auto entry1new =
      state3->getVlans()->getNode(host1vlan)->getArpTable()->getEntry(host1ip);
  auto entry2new =
      state3->getVlans()->getNode(host2vlan)->getArpTable()->getEntry(host2ip);
  auto entry3new =
      state3->getVlans()->getNode(host3vlan)->getNdpTable()->getEntry(host3ip);

  shared_ptr<SwitchState> state4{state3};
  // state3
  //  ... revert host2's resolved entry (to unresolved entry)
  // state4
  std::set<tuple<VlanID, IPAddressV4, shared_ptr<ArpEntry>>> changed4;
  SwitchState::revertNewNeighborEntry<ArpEntry, ArpTable>(
      entry2new, entry2old, &state4);
  changed4.insert(make_tuple(host2vlan, host2ip, entry2old));
  state4->publish();
  checkChangedNeighborEntries<ArpTable>(state3, state4, changed4, {}, {});

  shared_ptr<SwitchState> state5{state4};
  // state4
  //  ... revert host3's resolved entry
  // state5
  std::set<tuple<VlanID, IPAddressV6, shared_ptr<NdpEntry>>> changed6;
  SwitchState::revertNewNeighborEntry<NdpEntry, NdpTable>(
      entry3new, entry3old, &state5);
  changed6.insert(make_tuple(host3vlan, host3ip, entry3old));
  state5->publish();
  checkChangedNeighborEntries<NdpTable>(state4, state5, changed6, {}, {});
}

TEST(SwitchStatePruningTests, ModifyState) {
  auto platform = createMockPlatform();
  SwitchConfig config;
  // The empty state
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();

  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)},
      {PortID(4), VlanID(22)},
      {PortID(5), VlanID(22)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")},
      {VlanID(22), MacAddress("fa:ce:b0:0c:22:00")}};
  // state0
  // ... register ports, vlans, interfaces
  // state1
  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  ASSERT_NE(state1, nullptr);
  ASSERT_TRUE(!state1->isPublished());
  state1->publish();

  // state1 does not have any arp table. Let us create an arp table and add it
  // to the state.
  shared_ptr<SwitchState> state2{state1};
  shared_ptr<ArpTable> freshArpTable = make_shared<ArpTable>();

  auto host1ip = IPAddressV4("10.0.21.1");
  auto host1mac = MacAddress("fa:ce:b0:0c:21:01");
  auto host1port = PortDescriptor(PortID(1));
  auto host1intf = InterfaceID(21);
  auto host1vlan = VlanID(21);
  // Add host1 (resolved) for vlan21
  freshArpTable->addEntry(host1ip, host1mac, host1port, host1intf);
  shared_ptr<Vlan> vlan1 = state1->getVlans()->getNode(host1vlan);
  ASSERT_TRUE(state1 == state2); // point to same state
  auto vlanPtr =
      state1->getVlans()->getNode(host1vlan)->modify(&state2, scope());
  vlanPtr->setArpTable(std::move(freshArpTable));
  ASSERT_TRUE(vlan1.get() != vlanPtr);
  shared_ptr<Vlan> vlan2 = state2->getVlans()->getNode(host1vlan);
  ASSERT_TRUE(vlan1 != vlan2);
}

// Test we can modify empty arp table without resetting the arp table to a new
// one created outside.
TEST(SwitchStatePruningTests, ModifyEmptyArpTable) {
  auto platform = createMockPlatform();
  SwitchConfig config;
  // state0 = the empty state
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();

  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)},
      {PortID(4), VlanID(22)},
      {PortID(5), VlanID(22)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")},
      {VlanID(22), MacAddress("fa:ce:b0:0c:22:00")}};
  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);
  // state0
  // ... register ports, vlans, interfaces
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  ASSERT_NE(state1, nullptr);
  ASSERT_TRUE(!state1->isPublished());
  state1->publish();

  shared_ptr<SwitchState> state2{state1};
  auto host1ip = IPAddressV4("10.0.21.1");
  auto host1mac = MacAddress("fa:ce:b0:0c:21:01");
  auto host1port = PortDescriptor(PortID(1));
  auto host1intf = InterfaceID(21);
  auto host1vlan = VlanID(21);
  // Old arp table
  auto arp1 = state1->getVlans()->getNode(host1vlan)->getArpTable();
  ASSERT_TRUE(state2->isPublished());
  auto arp1modified =
      state1->getVlans()->getNode(host1vlan)->getArpTable()->modify(
          host1vlan, &state2);
  ASSERT_TRUE(!state2->isPublished());
  arp1modified->addEntry(host1ip, host1mac, host1port, host1intf);

  auto arp2 = state2->getVlans()->getNode(host1vlan)->getArpTable();
  ASSERT_TRUE(arp1 != arp2);
  ASSERT_TRUE(!arp2->isPublished());
  ASSERT_TRUE(arp2->getGeneration() == arp1->getGeneration() + 1);

  state2->publish();
  ASSERT_TRUE(arp2->isPublished());
}

/**
 * This code tests that the modify function of NeighborTable works as expected.
 */
TEST(SwitchStatePruningTests, ModifyArpTableMultipleTimes) {
  auto platform = createMockPlatform();
  SwitchConfig config;
  // The empty state
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();

  // Add some ports and corresponding Vlans
  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)},
      {PortID(4), VlanID(22)},
      {PortID(5), VlanID(22)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")},
      {VlanID(22), MacAddress("fa:ce:b0:0c:22:00")}};
  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);
  // state0
  //  ... register ports, vlans, interfaces
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  ASSERT_NE(state1, nullptr);
  ASSERT_TRUE(!state1->isPublished());
  state1->publish();

  shared_ptr<SwitchState> state2{state1};
  // state1
  //  ... add host1 to vlan21
  // state2

  auto host1ip = IPAddressV4("10.0.21.1");
  auto host1mac = MacAddress("fa:ce:b0:0c:21:01");
  auto host1port = PortDescriptor(PortID(1));
  auto host1intf = InterfaceID(21);
  auto host1vlan = VlanID(21);
  auto arp1 = state1->getVlans()->getNode(host1vlan)->getArpTable();
  ASSERT_EQ(state1, state2);
  ASSERT_TRUE(state2->isPublished());
  // Make sure that the modify function on ArpTable modifies the SwitchState
  // and VlanMap and Vlan and the mofified new state is unpublished.
  auto arp1modified =
      state1->getVlans()->getNode(host1vlan)->getArpTable()->modify(
          host1vlan, &state2);
  ASSERT_NE(state1, state2);
  ASSERT_NE(state1->getVlans(), state2->getVlans());
  ASSERT_NE(
      state1->getVlans()->getNode(host1vlan),
      state2->getVlans()->getNode(host1vlan));
  ASSERT_NE(
      state1->getVlans()->getNode(host1vlan)->getArpTable(),
      state2->getVlans()->getNode(host1vlan)->getArpTable());
  ASSERT_NE(arp1.get(), arp1modified);
  ASSERT_TRUE(!state2->isPublished());

  shared_ptr<SwitchState> state3{state2};
  // state2 (unpublished)
  //  ... add host4 to vlan22
  // state3

  auto host4ip = IPAddressV4("10.0.22.4");
  auto host4mac = MacAddress("fa:ce:b0:0c:22:04");
  auto host4port = PortDescriptor(PortID(4));
  auto host4intf = InterfaceID(22);
  auto host4vlan = VlanID(22);
  // Make sure the modify function does not change the SwitchState and VlanMap
  // if the state is unpublished.
  auto arpTable22Ptr =
      state2->getVlans()->getNode(host4vlan)->getArpTable()->modify(
          host4vlan, &state3);
  ASSERT_EQ(state2, state3);
  ASSERT_EQ(state2->getVlans(), state3->getVlans());
  arpTable22Ptr->addEntry(host4ip, host4mac, host4port, host4intf);
  state3->publish();
  ASSERT_EQ(state2, state3);
  ASSERT_EQ(state2->getVlans(), state3->getVlans());
}
