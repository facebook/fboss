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

template <typename VlanOrIntfIDT>
auto getVlansOrIntfs(const shared_ptr<SwitchState> state) {
  if constexpr (std::is_same_v<VlanOrIntfIDT, VlanID>) {
    return state->getVlans();
  } else {
    return state->getInterfaces();
  }
}

template <typename VlanOrIntfIDT>
auto getVlanOrIntfMapDelta(const StateDelta& delta) {
  if constexpr (std::is_same_v<VlanOrIntfIDT, VlanID>) {
    return delta.getVlansDelta();
  } else {
    return delta.getIntfsDelta();
  }
}

} // namespace

template <typename VlanOrIntfIDT, typename NTableT>
void checkChangedNeighborEntries(
    const shared_ptr<SwitchState>& state,
    const shared_ptr<SwitchState>& prunedState,
    const std::set<tuple<
        VlanOrIntfIDT,
        typename NTableT::Entry::AddressType,
        shared_ptr<typename NTableT::Entry>>>& changedIPs,
    const std::set<pair<VlanOrIntfIDT, typename NTableT::Entry::AddressType>>&
        addedIPs,
    const std::set<pair<VlanOrIntfIDT, typename NTableT::Entry::AddressType>>&
        removedIPs) {
  using EntryT = typename NTableT::Entry;
  using AddressT = typename EntryT::AddressType;
  CHECK(addedIPs.empty());
  // We never add things to state when we prune a state
  StateDelta delta(state, prunedState);
  std::set<tuple<VlanOrIntfIDT, AddressT, shared_ptr<EntryT>>> changedIPsFound;
  std::set<pair<VlanOrIntfIDT, AddressT>> addedIPsFound;
  std::set<pair<VlanOrIntfIDT, AddressT>> removedIPsFound;

  for (const auto& vlanOrIntfDelta :
       getVlanOrIntfMapDelta<VlanOrIntfIDT>(delta)) {
    for (const auto& entry :
         vlanOrIntfDelta.template getNeighborDelta<NTableT>()) {
      auto entryBeforePrune = entry.getOld();
      auto entryAfterPrune = entry.getNew();
      CHECK(entryBeforePrune || entryAfterPrune);
      if (entryBeforePrune && entryAfterPrune) {
        // changed
        CHECK_EQ(entryBeforePrune->getIP(), entryAfterPrune->getIP());
        changedIPsFound.insert(make_tuple(
            static_cast<VlanOrIntfIDT>(entryAfterPrune->getIntfID()),
            entryAfterPrune->getIP(),
            entryAfterPrune));
      } else if (entryBeforePrune) {
        // removed
        removedIPsFound.insert(make_pair(
            static_cast<VlanOrIntfIDT>(entryBeforePrune->getIntfID()),
            entryBeforePrune->getIP()));
      } else {
        // added
        addedIPsFound.insert(make_pair(
            static_cast<VlanOrIntfIDT>(entryAfterPrune->getIntfID()),
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
template <typename VlanOrIntfT, typename NTable>
void addNeighborEntry(
    shared_ptr<SwitchState>* state, // published or unpublished
    typename NTable::AddressType* ip,
    MacAddress* mac,
    VlanOrIntfT* vlanOrIntf,
    PortDescriptor* port) { // null port indicates pending entry
  CHECK(ip);
  CHECK(vlanOrIntf);
  shared_ptr<NTable> newNeighborTable = make_shared<NTable>();
  NTable* newNeighborTablePtr = newNeighborTable.get();
  auto oldNeighborTable = getVlansOrIntfs<VlanOrIntfT>(*state)
                              ->getNode(*vlanOrIntf)
                              ->template getNeighborTable<NTable>();

  if (oldNeighborTable) {
    // This call changes state too
    newNeighborTablePtr = oldNeighborTable->modify(*vlanOrIntf, state);
  }

  // Add entry to the arp table
  if (port) {
    newNeighborTablePtr->addEntry(
        *ip, *mac, *port, static_cast<InterfaceID>(*vlanOrIntf));
  } else {
    newNeighborTablePtr->addPendingEntry(
        *ip, static_cast<InterfaceID>(*vlanOrIntf));
  }

  if (!oldNeighborTable) {
    // There is no old arp table
    auto vlanOrIntfPtr =
        getVlansOrIntfs<VlanOrIntfT>(*state)->getNode(*vlanOrIntf).get();
    vlanOrIntfPtr = vlanOrIntfPtr->modify(state);
    vlanOrIntfPtr->setNeighborTable(std::move(newNeighborTable));
  }
}

// Add NeighborEntry to given state. The added entry can be pending or resolved
// based on port value (nullptr indicates pending entry).
template <typename NTable>
void addNeighborEntryToIntfNeighborTable(
    shared_ptr<SwitchState>* state, // published or unpublished
    typename NTable::AddressType* ip,
    MacAddress* mac,
    InterfaceID* intfID,
    PortDescriptor* port) { // null port indicates pending entry
  CHECK(ip);
  CHECK(intfID);

  shared_ptr<NTable> newNeighborTable = make_shared<NTable>();
  NTable* newNeighborTablePtr = newNeighborTable.get();

  auto oldNeighborTable = (*state)
                              ->getInterfaces()
                              ->getNode(*intfID)
                              ->template getNeighborTable<NTable>();

  if (oldNeighborTable) {
    newNeighborTablePtr = oldNeighborTable->modify(*intfID, state);
  }

  if (port) {
    newNeighborTablePtr->addEntry(*ip, *mac, *port, *intfID);
  } else {
    newNeighborTablePtr->addPendingEntry(*ip, *intfID);
  }

  if (!oldNeighborTable) {
    auto intfPtr = (*state)->getInterfaces()->getNode(*intfID).get();
    intfPtr = intfPtr->modify(state);

    if constexpr (std::is_same_v<NTable, ArpTable>) {
      intfPtr->setArpTable(std::move(newNeighborTable));
    } else {
      intfPtr->setNdpTable(std::move(newNeighborTable));
    }
  }
}

template <typename VlanOrIntfT>
void verifyAddNeighborEntry(
    VlanOrIntfT host1VlanOrIntf,
    VlanOrIntfT host2VlanOrIntf,
    VlanOrIntfT host3VlanOrIntf) {
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
  auto host1port = PortDescriptor(PortID(1));
  // Add host1 (resolved) for vlan21
  addNeighborEntry<VlanOrIntfT, ArpTable>(
      &state2, &host1ip, &host1mac, &host1VlanOrIntf, &host1port);

  auto host2ip = IPAddressV6("face:b00c:0:21::2");
  auto host2mac = MacAddress("fa:ce:b0:0c:21:02");
  auto host2port = PortDescriptor(PortID(1));

  addNeighborEntry<VlanOrIntfT, NdpTable>(
      &state2, &host2ip, &host2mac, &host2VlanOrIntf, &host2port);

  state2->publish();

  shared_ptr<SwitchState> state3{state2};
  // state2
  //  ... add host3 to vlan21's arp table
  // state3
  // Add a new entry

  auto host3ip = IPAddressV4("10.0.21.3");
  auto host3mac = MacAddress("fa:ce:b0:0c:21:03");
  auto host3port = PortDescriptor(PortID(2));
  addNeighborEntry<VlanOrIntfT, ArpTable>(
      &state3, &host3ip, &host3mac, &host3VlanOrIntf, &host3port);
  state3->publish();

  auto state4 = state3;
  // state3
  //  ... prune host3 from vlan21
  // state4

  set<pair<VlanOrIntfT, IPAddressV4>> removedV4;
  // Remove host3 from the arp table

  auto h3entry = getVlansOrIntfs<VlanOrIntfT>(state3)
                     ->getNode(host3VlanOrIntf)
                     ->getArpTable()
                     ->getEntry(host3ip);

  SwitchState::revertNewNeighborEntry<ArpEntry, ArpTable>(
      h3entry, nullptr, &state4);
  removedV4.insert(make_pair(host3VlanOrIntf, host3ip));
  checkChangedNeighborEntries<VlanOrIntfT, ArpTable>(
      state3, state4, {} /* no changed */, {} /* no added */, removedV4);

  state4->publish();

  auto state5 = state4;
  // state4
  //  ... prune host2 from vlan21 (ipv6 example)
  // state5
  set<pair<VlanOrIntfT, IPAddressV6>> removedV6;
  auto v6entry = getVlansOrIntfs<VlanOrIntfT>(state4)
                     ->getNode(host2VlanOrIntf)
                     ->getNdpTable()
                     ->getEntry(host2ip);
  SwitchState::revertNewNeighborEntry<NdpEntry, NdpTable>(
      v6entry, nullptr, &state5);
  removedV6.insert(make_pair(host2VlanOrIntf, host2ip));
  checkChangedNeighborEntries<VlanOrIntfT, NdpTable>(
      state4, state5, {}, {}, removedV6);

  state5->publish();
}

// Test that we can add Arp and Ndp entries to a state and revert them from the
// published state.
TEST(SwitchStatePruningTests, AddNeighborEntry) {
  FLAGS_intf_nbr_tables = false;
  verifyAddNeighborEntry(
      VlanID(21) /* host1 */, VlanID(21) /* host2 */, VlanID(21) /* host3 */);

  FLAGS_intf_nbr_tables = true;
  verifyAddNeighborEntry(
      InterfaceID(21) /* host1 */,
      InterfaceID(21) /* host2 */,
      InterfaceID(21) /* host3 */);
}

template <typename VlanOrIntfT>
void verifyChangeNeighborEntry(
    VlanOrIntfT host1VlanOrIntf,
    VlanOrIntfT host2VlanOrIntf,
    VlanOrIntfT host3VlanOrIntf) {
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
  addNeighborEntry<VlanOrIntfT, ArpTable>(
      &state2, &host1ip, nullptr, &host1VlanOrIntf, nullptr);

  auto host2ip = IPAddressV4("10.0.21.2");
  addNeighborEntry<VlanOrIntfT, ArpTable>(
      &state2, &host2ip, nullptr, &host2VlanOrIntf, nullptr);

  auto host3ip = IPAddressV6("face:b00c:0:22::3");
  addNeighborEntry<VlanOrIntfT, NdpTable>(
      &state2, &host3ip, nullptr, &host3VlanOrIntf, nullptr);

  state2->publish();

  auto entry1old = getVlansOrIntfs<VlanOrIntfT>(state2)
                       ->getNode(host1VlanOrIntf)
                       ->getArpTable()
                       ->getEntry(host1ip);
  auto entry2old = getVlansOrIntfs<VlanOrIntfT>(state2)
                       ->getNode(host2VlanOrIntf)
                       ->getArpTable()
                       ->getEntry(host2ip);
  auto entry3old = getVlansOrIntfs<VlanOrIntfT>(state2)
                       ->getNode(host3VlanOrIntf)
                       ->getNdpTable()
                       ->getEntry(host3ip);

  auto state3 = state2;
  // state2
  //  ... resolve arp entry for host2
  //  ... resolve ndp entry for host3
  // state3

  auto host2mac = MacAddress("fa:ce:b0:0c:21:02");
  auto host2port = PortDescriptor(PortID(2));
  auto host2interface = InterfaceID(21);

  auto arpTable21 = getVlansOrIntfs<VlanOrIntfT>(state3)
                        ->getNode(host2VlanOrIntf)
                        ->getArpTable()
                        ->modify(host2VlanOrIntf, &state3);

  arpTable21->updateEntry(
      host2ip, host2mac, host2port, host2interface, NeighborState::REACHABLE);

  auto host3mac = MacAddress("fa:ce:b0:0c:22:03");
  auto host3port = PortDescriptor(PortID(4));
  auto host3interface = InterfaceID(22);

  auto ndpTable22 = getVlansOrIntfs<VlanOrIntfT>(state3)
                        ->getNode(host3VlanOrIntf)
                        ->getNdpTable()
                        ->modify(host3VlanOrIntf, &state3);
  ndpTable22->updateEntry(
      host3ip, host3mac, host3port, host3interface, NeighborState::REACHABLE);

  state3->publish();

  auto entry1new = getVlansOrIntfs<VlanOrIntfT>(state3)
                       ->getNode(host1VlanOrIntf)
                       ->getArpTable()
                       ->getEntry(host1ip);
  auto entry2new = getVlansOrIntfs<VlanOrIntfT>(state3)
                       ->getNode(host2VlanOrIntf)
                       ->getArpTable()
                       ->getEntry(host2ip);
  auto entry3new = getVlansOrIntfs<VlanOrIntfT>(state3)
                       ->getNode(host3VlanOrIntf)
                       ->getNdpTable()
                       ->getEntry(host3ip);

  shared_ptr<SwitchState> state4{state3};
  // state3
  //  ... revert host2's resolved entry (to unresolved entry)
  // state4
  std::set<tuple<VlanOrIntfT, IPAddressV4, shared_ptr<ArpEntry>>> changed4;
  SwitchState::revertNewNeighborEntry<ArpEntry, ArpTable>(
      entry2new, entry2old, &state4);
  changed4.insert(make_tuple(host2VlanOrIntf, host2ip, entry2old));
  state4->publish();
  checkChangedNeighborEntries<VlanOrIntfT, ArpTable>(
      state3, state4, changed4, {}, {});

  shared_ptr<SwitchState> state5{state4};
  // state4
  //  ... revert host3's resolved entry
  // state5
  std::set<tuple<VlanOrIntfT, IPAddressV6, shared_ptr<NdpEntry>>> changed6;
  SwitchState::revertNewNeighborEntry<NdpEntry, NdpTable>(
      entry3new, entry3old, &state5);
  changed6.insert(make_tuple(host3VlanOrIntf, host3ip, entry3old));
  state5->publish();
  checkChangedNeighborEntries<VlanOrIntfT, NdpTable>(
      state4, state5, changed6, {}, {});
}

// Test that we can update pending entries to resolved ones, and revert them
// back to pending.
TEST(SwitchStatePruningTests, ChangeNeighborEntry) {
  FLAGS_intf_nbr_tables = false;
  verifyChangeNeighborEntry<VlanID>(
      VlanID(21) /* host1 */, VlanID(21) /* host2 */, VlanID(22) /* host3 */);

  FLAGS_intf_nbr_tables = true;
  verifyChangeNeighborEntry<InterfaceID>(
      InterfaceID(21) /* host1 */,
      InterfaceID(21) /* host2 */,
      InterfaceID(22) /* host3 */);
}

template <typename VlanOrIntfT>
void verifyModifyState(VlanOrIntfT host1VlanOrIntf) {
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
  // Add host1 (resolved) for vlan21
  freshArpTable->addEntry(
      host1ip, host1mac, host1port, static_cast<InterfaceID>(host1VlanOrIntf));

  auto vlanOrIntf1 =
      getVlansOrIntfs<VlanOrIntfT>(state1)->getNode(host1VlanOrIntf);
  ASSERT_TRUE(state1 == state2); // point to same state

  auto vlanOrIntfPtr = getVlansOrIntfs<VlanOrIntfT>(state1)
                           ->getNode(host1VlanOrIntf)
                           ->modify(&state2);
  vlanOrIntfPtr->setArpTable(std::move(freshArpTable));
  ASSERT_TRUE(vlanOrIntf1.get() != vlanOrIntfPtr);

  auto vlanOrIntf2 =
      getVlansOrIntfs<VlanOrIntfT>(state2)->getNode(host1VlanOrIntf);
  ASSERT_TRUE(vlanOrIntf1 != vlanOrIntf2);
}

TEST(SwitchStatePruningTests, ModifyState) {
  FLAGS_intf_nbr_tables = false;
  verifyModifyState(VlanID(21));

  FLAGS_intf_nbr_tables = true;
  verifyModifyState(InterfaceID(21));
}

template <typename VlanOrIntfT>
void verifyModifyEmptyArpTable(VlanOrIntfT host1VlanOrIntf) {
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
  // Old arp table
  auto arp1 = getVlansOrIntfs<VlanOrIntfT>(state1)
                  ->getNode(host1VlanOrIntf)
                  ->getArpTable();
  ASSERT_TRUE(state2->isPublished());
  auto arp1modified = getVlansOrIntfs<VlanOrIntfT>(state1)
                          ->getNode(host1VlanOrIntf)
                          ->getArpTable()
                          ->modify(host1VlanOrIntf, &state2);
  ASSERT_TRUE(!state2->isPublished());
  arp1modified->addEntry(
      host1ip, host1mac, host1port, static_cast<InterfaceID>(host1VlanOrIntf));

  auto arp2 = getVlansOrIntfs<VlanOrIntfT>(state2)
                  ->getNode(host1VlanOrIntf)
                  ->getArpTable();
  ASSERT_TRUE(arp1 != arp2);
  ASSERT_TRUE(!arp2->isPublished());
  ASSERT_TRUE(arp2->getGeneration() == arp1->getGeneration() + 1);

  state2->publish();
  ASSERT_TRUE(arp2->isPublished());
}

// Test we can modify empty arp table without resetting the arp table to a new
// one created outside.
TEST(SwitchStatePruningTests, ModifyEmptyArpTable) {
  FLAGS_intf_nbr_tables = false;
  verifyModifyEmptyArpTable(VlanID(21));

  FLAGS_intf_nbr_tables = true;
  verifyModifyEmptyArpTable(InterfaceID(21));
}

template <typename VlanOrIntfT>
void verifyModifyArpTableMultipleTimes(
    VlanOrIntfT host1VlanOrIntf,
    VlanOrIntfT host4VlanOrIntf) {
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

  auto arp1 = getVlansOrIntfs<VlanOrIntfT>(state1)
                  ->getNode(host1VlanOrIntf)
                  ->getArpTable();
  ASSERT_EQ(state1, state2);
  ASSERT_TRUE(state2->isPublished());

  // Make sure that the modify function on ArpTable modifies the SwitchState
  // and VlanMap and Vlan and the mofified new state is unpublished.
  auto arp1modified = getVlansOrIntfs<VlanOrIntfT>(state1)
                          ->getNode(host1VlanOrIntf)
                          ->getArpTable()
                          ->modify(host1VlanOrIntf, &state2);
  ASSERT_NE(state1, state2);
  ASSERT_NE(
      getVlansOrIntfs<VlanOrIntfT>(state1),
      getVlansOrIntfs<VlanOrIntfT>(state2));
  ASSERT_NE(
      getVlansOrIntfs<VlanOrIntfT>(state1)
          ->getNode(host1VlanOrIntf)
          ->getArpTable(),
      getVlansOrIntfs<VlanOrIntfT>(state2)
          ->getNode(host1VlanOrIntf)
          ->getArpTable());

  ASSERT_NE(arp1.get(), arp1modified);
  ASSERT_TRUE(!state2->isPublished());

  ASSERT_NE(
      getVlansOrIntfs<VlanOrIntfT>(state1)
          ->getNode(host1VlanOrIntf)
          ->getArpTable(),
      getVlansOrIntfs<VlanOrIntfT>(state2)
          ->getNode(host1VlanOrIntf)
          ->getArpTable());

  ASSERT_NE(arp1.get(), arp1modified);
  ASSERT_TRUE(!state2->isPublished());

  shared_ptr<SwitchState> state3{state2};
  // state2 (unpublished)
  //  ... add host4 to vlan22
  // state3

  auto host4ip = IPAddressV4("10.0.22.4");
  auto host4mac = MacAddress("fa:ce:b0:0c:22:04");
  auto host4port = PortDescriptor(PortID(4));

  // Make sure the modify function does not change the SwitchState and VlanMap
  // if the state is unpublished.
  auto arpTable22Ptr = getVlansOrIntfs<VlanOrIntfT>(state2)
                           ->getNode(host4VlanOrIntf)
                           ->getArpTable()
                           ->modify(host4VlanOrIntf, &state3);
  ASSERT_EQ(state2, state3);
  ASSERT_EQ(
      getVlansOrIntfs<VlanOrIntfT>(state2),
      getVlansOrIntfs<VlanOrIntfT>(state3));

  arpTable22Ptr->addEntry(
      host4ip, host4mac, host4port, static_cast<InterfaceID>(host4VlanOrIntf));
  state3->publish();
  ASSERT_EQ(state2, state3);
  ASSERT_EQ(
      getVlansOrIntfs<VlanOrIntfT>(state2),
      getVlansOrIntfs<VlanOrIntfT>(state3));
}

/**
 * This code tests that the modify function of NeighborTable works as expected.
 */
TEST(SwitchStatePruningTests, ModifyArpTableMultipleTimes) {
  FLAGS_intf_nbr_tables = false;
  verifyModifyArpTableMultipleTimes(VlanID(21), VlanID(22));

  FLAGS_intf_nbr_tables = true;
  verifyModifyArpTableMultipleTimes(InterfaceID(21), InterfaceID(22));
}

shared_ptr<SwitchState> createSwitch() {
  auto platform = createMockPlatform();
  SwitchConfig config;
  shared_ptr<SwitchState> state0 = make_shared<SwitchState>();

  std::map<PortID, VlanID> port2VlanMap = {
      {PortID(1), VlanID(21)},
      {PortID(2), VlanID(21)},
      {PortID(3), VlanID(21)}};
  std::map<VlanID, MacAddress> vlan2OutgoingMac = {
      {VlanID(21), MacAddress("fa:ce:b0:0c:21:00")}};

  registerPortsAndPopulateConfig(
      port2VlanMap, &vlan2OutgoingMac, state0, config);

  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  state1->publish();

  return state1;
}

shared_ptr<SwitchState> addNeighbors(
    shared_ptr<SwitchState> state1,
    bool vlanNeighbors) {
  shared_ptr<SwitchState> state2{state1};

  auto host1Ip = IPAddressV4("10.0.21.1");
  auto host1Mac = MacAddress("fa:ce:b0:0c:21:01");
  auto host2Ip = IPAddressV6("face:b00c:0:21::2");
  auto host2Mac = MacAddress("fa:ce:b0:0c:21:02");

  auto hostVlan = VlanID(21);
  auto hostIntf = InterfaceID(21);
  auto hostPort = PortDescriptor(PortID(1));

  if (vlanNeighbors) {
    addNeighborEntry<VlanID, ArpTable>(
        &state2, &host1Ip, &host1Mac, &hostVlan, &hostPort);
    addNeighborEntry<VlanID, NdpTable>(
        &state2, &host2Ip, &host2Mac, &hostVlan, &hostPort);
  } else {
    addNeighborEntryToIntfNeighborTable<ArpTable>(
        &state2, &host1Ip, &host1Mac, &hostIntf, &hostPort);
    addNeighborEntryToIntfNeighborTable<NdpTable>(
        &state2, &host2Ip, &host2Mac, &hostIntf, &hostPort);
  }

  state2->publish();
  return state2;
}

template <typename MultiMapT>
void verifyNbrTablesEmpty(const shared_ptr<MultiMapT> multiMap) {
  for (const auto& table : *multiMap) {
    for (const auto& [_, vlanOrIntf] : *table.second) {
      EXPECT_TRUE(
          vlanOrIntf->getArpTable() == nullptr ||
          vlanOrIntf->getArpTable()->size() == 0);
      EXPECT_TRUE(
          vlanOrIntf->getNdpTable() == nullptr ||
          vlanOrIntf->getNdpTable()->size() == 0);
      EXPECT_TRUE(
          vlanOrIntf->getArpResponseTable() == nullptr ||
          vlanOrIntf->getArpResponseTable()->size() == 0);
      EXPECT_TRUE(
          vlanOrIntf->getNdpResponseTable() == nullptr ||
          vlanOrIntf->getNdpResponseTable()->size() == 0);
    }
  }
}

template <typename MultiMapT>
void verifyNbrTablesNonEmpty(const shared_ptr<MultiMapT> multiMap) {
  for (const auto& table : *multiMap) {
    for (const auto& [_, vlanOrIntf] : *table.second) {
      EXPECT_NE(vlanOrIntf->getArpTable(), nullptr);
      EXPECT_EQ(vlanOrIntf->getArpTable()->size(), 1);
      EXPECT_EQ(
          vlanOrIntf->getArpTable()->cbegin()->second->getIP(),
          IPAddressV4("10.0.21.1"));

      EXPECT_NE(vlanOrIntf->getNdpTable(), nullptr);
      EXPECT_EQ(vlanOrIntf->getNdpTable()->size(), 1);
      EXPECT_EQ(
          vlanOrIntf->getNdpTable()->cbegin()->second->getIP(),
          IPAddressV6("face:b00c:0:21::2"));
    }
  }
}

TEST(SwitchStatePruningTests, VlanNbrTablesWbToVlanNbrTables) {
  FLAGS_intf_nbr_tables = false;
  auto state = addNeighbors(createSwitch(), true /* vlan neighbors */);
  auto thrifty = state->toThrift();

  auto thriftStateBack = SwitchState::fromThrift(thrifty);
  verifyNbrTablesNonEmpty(thriftStateBack->getVlans());
  verifyNbrTablesEmpty(thriftStateBack->getInterfaces());
}

TEST(SwitchStatePruningTests, VlanNbrTablesWbToIntfNbrTables) {
  FLAGS_intf_nbr_tables = false;
  auto state = addNeighbors(createSwitch(), true /* vlan neighbors */);
  auto thrifty = state->toThrift();

  FLAGS_intf_nbr_tables = true;
  auto thriftStateBack = SwitchState::fromThrift(thrifty);
  verifyNbrTablesEmpty(thriftStateBack->getVlans());
  verifyNbrTablesNonEmpty(thriftStateBack->getInterfaces());
}

TEST(SwitchStatePruningTests, IntfNbrTablesWbVlanNbrTables) {
  FLAGS_intf_nbr_tables = true;
  auto state = addNeighbors(createSwitch(), false /* intf neighbors */);
  auto thrifty = state->toThrift();

  FLAGS_intf_nbr_tables = false;
  auto thriftStateBack = SwitchState::fromThrift(thrifty);
  verifyNbrTablesNonEmpty(thriftStateBack->getVlans());
  verifyNbrTablesEmpty(thriftStateBack->getInterfaces());
}

TEST(SwitchStatePruningTests, IntfNbrTablesWbToIntfNbrTables) {
  FLAGS_intf_nbr_tables = true;
  auto state = addNeighbors(createSwitch(), false /* intf neighbors */);
  auto thrifty = state->toThrift();

  auto thriftStateBack = SwitchState::fromThrift(thrifty);
  verifyNbrTablesEmpty(thriftStateBack->getVlans());
  verifyNbrTablesNonEmpty(thriftStateBack->getInterfaces());
}
