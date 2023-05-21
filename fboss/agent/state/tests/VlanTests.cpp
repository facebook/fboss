/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <string>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;
using std::string;
using testing::Return;

namespace {
static const string kVlan1234("Vlan1234");
static const string kVlan99("Vlan99");
static const string kVlan1299("Vlan1299");

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

template <typename Entry>
void validateRespEntry(
    std::shared_ptr<Entry> entry,
    folly::MacAddress mac,
    InterfaceID intf) {
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->getMac(), mac);
  EXPECT_EQ(entry->getInterfaceID(), intf);
}

TEST(Vlan, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto vlanV0 = make_shared<Vlan>(VlanID(1234), kVlan1234);

  stateV0->addVlan(vlanV0);
  registerPort(stateV0, PortID(1), "port1", scope());
  registerPort(stateV0, PortID(2), "port2", scope());

  NodeID nodeID = vlanV0->getNodeID();
  EXPECT_EQ(0, vlanV0->getGeneration());
  EXPECT_FALSE(vlanV0->isPublished());
  EXPECT_EQ(VlanID(1234), vlanV0->getID());
  Vlan::MemberPorts emptyPorts;
  EXPECT_EQ(emptyPorts, vlanV0->getPorts());

  vlanV0->publish();
  EXPECT_TRUE(vlanV0->isPublished());

  cfg::SwitchConfig config;
  config.ports()->resize(2);
  preparedMockPortConfig(config.ports()[0], 1);
  preparedMockPortConfig(config.ports()[1], 2);
  config.vlans()->resize(1);
  config.vlans()[0].id() = 1234;
  config.vlans()[0].name() = kVlan1234;
  config.vlans()[0].dhcpRelayOverridesV4() = {};
  (*config.vlans()[0].dhcpRelayOverridesV4())["02:00:00:00:00:02"] = "1.2.3.4";
  config.vlans()[0].dhcpRelayOverridesV6() = {};
  (*config.vlans()[0].dhcpRelayOverridesV6())["02:00:00:00:00:02"] =
      "2a03:2880:10:1f07:face:b00c:0:0";
  config.vlans()[0].intfID() = 1234;
  config.vlanPorts()->resize(2);
  config.vlanPorts()[0].logicalPort() = 1;
  config.vlanPorts()[0].vlanID() = 1234;
  config.vlanPorts()[0].emitTags() = false;
  config.vlanPorts()[1].logicalPort() = 2;
  config.vlanPorts()[1].vlanID() = 1234;
  config.vlanPorts()[1].emitTags() = true;

  config.interfaces()->resize(1);
  config.interfaces()[0].intfID() = 1234;
  config.interfaces()[0].vlanID() = 1234;

  Vlan::MemberPorts expectedPorts;
  expectedPorts.insert(std::make_pair(1, false));
  expectedPorts.insert(std::make_pair(2, true));

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto vlanV1 = stateV1->getVlans()->getVlan(VlanID(1234));
  ASSERT_NE(nullptr, vlanV1);
  auto vlanV1_byName = stateV1->getVlans()->getVlanSlow(kVlan1234);
  EXPECT_EQ(vlanV1, vlanV1_byName);
  EXPECT_EQ(nodeID, vlanV1->getNodeID());
  EXPECT_EQ(1, vlanV1->getGeneration());
  EXPECT_FALSE(vlanV1->isPublished());
  EXPECT_EQ(VlanID(1234), vlanV1->getID());
  EXPECT_EQ(kVlan1234, vlanV1->getName());
  EXPECT_EQ(expectedPorts, vlanV1->getPorts());
  EXPECT_EQ(0, vlanV1->getArpResponseTable()->size());
  EXPECT_EQ(InterfaceID(1234), vlanV1->getInterfaceID());

  auto map4 = vlanV1->getDhcpV4RelayOverrides();
  EXPECT_EQ(
      IPAddressV4("1.2.3.4"),
      IPAddressV4(map4[MacAddress("02:00:00:00:00:02")]));
  auto map6 = vlanV1->getDhcpV6RelayOverrides();
  EXPECT_EQ(
      IPAddressV6("2a03:2880:10:1f07:face:b00c:0:0"),
      IPAddressV6(map6[MacAddress("02:00:00:00:00:02")]));

  // Applying the same config again should return null
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // Add an interface
  config.interfaces()->resize(2);
  *config.interfaces()[1].intfID() = 1;
  *config.interfaces()[1].routerID() = 0;
  *config.interfaces()[1].vlanID() = 1;
  config.interfaces()[1].ipAddresses()->resize(2);
  config.interfaces()[1].ipAddresses()[0] = "10.1.1.1/24";
  config.interfaces()[1].ipAddresses()[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";
  config.vlans()->resize(2);
  config.vlans()[1].id() = 1;
  config.vlans()[1].name() = "vlan1";
  config.vlans()[1].intfID() = 1;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  auto vlanV2 = stateV2->getVlans()->getVlan(VlanID(1));
  EXPECT_EQ(0, vlanV2->getGeneration());
  EXPECT_FALSE(vlanV2->isPublished());
  EXPECT_EQ(VlanID(1), vlanV2->getID());
  EXPECT_EQ("vlan1", vlanV2->getName());
  EXPECT_EQ(0, vlanV2->getPorts().size());
  EXPECT_EQ(InterfaceID(1), vlanV2->getInterfaceID());
  // Check the ArpResponseTable
  auto arpRespTable = vlanV2->getArpResponseTable();
  EXPECT_EQ(arpRespTable->size(), 1);
  validateRespEntry(
      arpRespTable->getEntry(IPAddressV4("10.1.1.1")),
      MockPlatform::getMockLocalMac(),
      InterfaceID(1));
  // Check the NdpResponseTable
  auto ndpRespTable = vlanV2->getNdpResponseTable();
  EXPECT_EQ(ndpRespTable->size(), 2);
  validateRespEntry(
      ndpRespTable->getEntry(IPAddressV6("2a03:2880:10:1f07:face:b00c:0:0")),
      MockPlatform::getMockLocalMac(),
      InterfaceID(1));

  // The link-local IPv6 address should also have been automatically added
  // to the NDP response table.
  validateRespEntry(
      ndpRespTable->getEntry(MockPlatform::getMockLinkLocalIp6()),
      MockPlatform::getMockLocalMac(),
      InterfaceID(1));

  // Add another vlan and interface
  config.vlans()->resize(3);
  *config.vlans()[2].id() = 1299;
  *config.vlans()[2].name() = kVlan1299;
  config.vlans()[2].intfID() = 1299;
  config.interfaces()->resize(3);
  *config.interfaces()[2].intfID() = 1299;
  *config.interfaces()[2].routerID() = 0;
  *config.interfaces()[2].vlanID() = 1299;
  config.interfaces()[2].ipAddresses()->resize(2);
  config.interfaces()[2].ipAddresses()[0] = "10.1.10.1/24";
  config.interfaces()[2].ipAddresses()[1] = "192.168.0.1/31";
  MacAddress intf2Mac("02:01:02:ab:cd:78");
  config.interfaces()[2].mac() = intf2Mac.toString();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto vlanV3 = stateV3->getVlans()->getVlan(VlanID(1299));
  EXPECT_EQ(0, vlanV3->getGeneration());
  EXPECT_FALSE(vlanV3->isPublished());
  EXPECT_EQ(VlanID(1299), vlanV3->getID());
  EXPECT_EQ(kVlan1299, vlanV3->getName());
  EXPECT_EQ(InterfaceID(1299), vlanV3->getInterfaceID());

  // Check the ArpResponseTable
  arpRespTable = vlanV3->getArpResponseTable();
  EXPECT_EQ(arpRespTable->size(), 2);
  validateRespEntry(
      arpRespTable->getEntry(IPAddressV4("10.1.10.1")),
      intf2Mac,
      InterfaceID(1299));
  validateRespEntry(
      arpRespTable->getEntry(IPAddressV4("192.168.0.1")),
      intf2Mac,
      InterfaceID(1299));

  // The new interface has no IPv6 address, but the NDP table should still
  // be updated with the link-local address.
  ndpRespTable = vlanV3->getNdpResponseTable();
  EXPECT_EQ(ndpRespTable->size(), 1);
  validateRespEntry(
      ndpRespTable->getEntry(IPAddressV6("fe80::1:02ff:feab:cd78")),
      intf2Mac,
      InterfaceID(1299));

  // Add a new VLAN with an ArpResponseTable that needs to be set up
  // when the VLAN is first created
  config.vlans()->resize(4);
  *config.vlans()[3].id() = 99;
  *config.vlans()[3].name() = kVlan99;
  config.vlans()[3].intfID() = 99;
  config.interfaces()->resize(4);
  *config.interfaces()[3].intfID() = 99;
  *config.interfaces()[3].routerID() = 1;
  *config.interfaces()[3].vlanID() = 99;
  config.interfaces()[3].ipAddresses()->resize(2);
  config.interfaces()[3].ipAddresses()[0] = "1.2.3.4/24";
  config.interfaces()[3].ipAddresses()[1] = "10.0.0.1/9";
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  // VLAN 1 should be unchanged
  EXPECT_EQ(vlanV2, stateV4->getVlans()->getVlan(VlanID(1)));
  auto vlan99 = stateV4->getVlans()->getVlan(VlanID(99));
  auto vlan99_byName = stateV4->getVlans()->getVlanSlow(kVlan99);
  ASSERT_NE(nullptr, vlan99);
  EXPECT_EQ(vlan99, vlan99_byName);
  EXPECT_EQ(0, vlan99->getGeneration());
  EXPECT_EQ(InterfaceID(99), vlan99->getInterfaceID());

  EXPECT_EQ(vlan99->getArpResponseTable()->size(), 2);
  validateRespEntry(
      vlan99->getArpResponseTable()->getEntry(IPAddressV4("1.2.3.4")),
      MockPlatform::getMockLocalMac(),
      InterfaceID(99));
  validateRespEntry(
      vlan99->getArpResponseTable()->getEntry(IPAddressV4("10.0.0.1")),
      MockPlatform::getMockLocalMac(),
      InterfaceID(99));

  // Check vlan congfig with no intfID set
  config.vlans()->resize(5);
  *config.vlans()[4].id() = 100;
  config.vlans()[4].intfID().reset();
  config.interfaces()->resize(5);
  *config.interfaces()[4].intfID() = 100;
  *config.interfaces()[4].routerID() = 0;
  *config.interfaces()[4].vlanID() = 100;
  config.interfaces()[4].ipAddresses()->resize(2);
  config.interfaces()[4].ipAddresses()[0] = "10.50.3.7/24";
  config.interfaces()[4].ipAddresses()[1] = "10.50.0.3/9";
  auto stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
  ASSERT_NE(nullptr, stateV5);
  auto vlan100 = stateV5->getVlans()->getVlan(VlanID(100));
  EXPECT_EQ(InterfaceID(100), vlan100->getInterfaceID());
}

/*
 * Test that forEachChanged(StateDelta::getVlansDelta(), ...) invokes the
 * callback for the specified list of changed VLANs.
 */
void checkChangedVlans(
    const shared_ptr<VlanMap>& oldVlans,
    const shared_ptr<VlanMap>& newVlans,
    const std::set<uint16_t> changedIDs,
    const std::set<uint16_t> addedIDs,
    const std::set<uint16_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetVlans(oldVlans);
  auto newState = make_shared<SwitchState>();
  newState->resetVlans(newVlans);

  std::set<uint16_t> foundChanged;
  std::set<uint16_t> foundAdded;
  std::set<uint16_t> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getVlansDelta(),
      [&](const shared_ptr<Vlan>& oldVlan, const shared_ptr<Vlan>& newVlan) {
        EXPECT_EQ(oldVlan->getID(), newVlan->getID());
        EXPECT_NE(oldVlan, newVlan);

        auto ret = foundChanged.insert(oldVlan->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<Vlan>& vlan) {
        auto ret = foundAdded.insert(vlan->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<Vlan>& vlan) {
        auto ret = foundRemoved.insert(vlan->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TEST(VlanMap, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  cfg::SwitchConfig config;

  std::vector<int> ports = {1, 2, 3, 4, 9, 19, 20};
  config.ports()->resize(ports.size());
  for (int i = 0; i < ports.size(); i++) {
    int port = ports[i];
    registerPort(
        stateV0, PortID(port), folly::format("port{}", port).str(), scope());

    preparedMockPortConfig(
        config.ports()[i],
        port,
        fmt::format("port{}", port),
        cfg::PortState::DISABLED);
  }

  auto vlansV0 = stateV0->getVlans();

  // Apply new config settings
  config.vlans()->resize(2);
  *config.vlans()[0].id() = 1234;
  *config.vlans()[0].name() = kVlan1234;
  *config.vlans()[1].id() = 99;
  *config.vlans()[1].name() = kVlan99;
  config.vlanPorts()->resize(7);
  *config.vlanPorts()[0].vlanID() = 1234;
  *config.vlanPorts()[0].logicalPort() = 1;
  *config.vlanPorts()[1].vlanID() = 1234;
  *config.vlanPorts()[1].logicalPort() = 2;
  *config.vlanPorts()[2].vlanID() = 1234;
  *config.vlanPorts()[2].logicalPort() = 3;
  *config.vlanPorts()[3].vlanID() = 1234;
  *config.vlanPorts()[3].logicalPort() = 4;
  *config.vlanPorts()[4].vlanID() = 99;
  *config.vlanPorts()[4].logicalPort() = 9;
  *config.vlanPorts()[5].vlanID() = 99;
  *config.vlanPorts()[5].logicalPort() = 19;
  *config.vlanPorts()[6].vlanID() = 99;
  *config.vlanPorts()[6].logicalPort() = 29;

  config.interfaces()->resize(2);
  *config.interfaces()[0].intfID() = 1234;
  *config.interfaces()[0].vlanID() = 1234;
  *config.interfaces()[0].routerID() = 0;
  *config.interfaces()[1].intfID() = 99;
  *config.interfaces()[1].vlanID() = 99;
  *config.interfaces()[1].routerID() = 0;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto vlansV1 = stateV1->getVlans();
  ASSERT_NE(nullptr, vlansV1);
  EXPECT_EQ(1, vlansV1->getGeneration());
  EXPECT_EQ(2, vlansV1->size());

  // Check the new settings for VLAN 1234
  auto vlan1234v0 = vlansV1->getVlan(VlanID(1234));
  auto vlan1234v0_byName = vlansV1->getVlanSlow(kVlan1234);
  ASSERT_NE(nullptr, vlan1234v0);
  EXPECT_EQ(vlan1234v0, vlan1234v0_byName);
  NodeID id1234 = vlan1234v0->getNodeID();
  EXPECT_EQ(VlanID(1234), vlan1234v0->getID());
  EXPECT_EQ(kVlan1234, vlan1234v0->getName());
  EXPECT_EQ(0, vlan1234v0->getGeneration());
  Vlan::MemberPorts ports1234v0;
  ports1234v0.insert(make_pair(1, false));
  ports1234v0.insert(make_pair(2, false));
  ports1234v0.insert(make_pair(3, false));
  ports1234v0.insert(make_pair(4, false));
  EXPECT_EQ(ports1234v0, vlan1234v0->getPorts());

  // Check the new settings for VLAN 99
  auto vlan99v0 = vlansV1->getVlan(VlanID(99));
  ASSERT_NE(nullptr, vlan99v0);
  auto vlan99v0_byName = vlansV1->getVlanSlow(kVlan99);
  EXPECT_EQ(vlan99v0, vlan99v0_byName);
  NodeID id99 = vlan99v0->getNodeID();
  EXPECT_NE(id1234, id99);
  EXPECT_EQ(VlanID(99), vlan99v0->getID());
  EXPECT_EQ(kVlan99, vlan99v0->getName());
  EXPECT_EQ(0, vlan99v0->getGeneration());
  Vlan::MemberPorts ports99v1;
  ports99v1.insert(make_pair(9, false));
  ports99v1.insert(make_pair(19, false));
  ports99v1.insert(make_pair(29, false));
  EXPECT_EQ(ports99v1, vlan99v0->getPorts());

  // getVlan() should throw on a non-existent VLAN
  EXPECT_THROW(vlansV1->getVlan(VlanID(1)), FbossError);
  // getVlanIf() should return null on a non-existent VLAN
  EXPECT_EQ(nullptr, vlansV1->getVlanIf(VlanID(1233)));

  checkChangedVlans(vlansV0, vlansV1, {}, {99, 1234}, {});

  // Applying the same config again should result in no change
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // Drop one port from VLAN 1234
  config.vlanPorts()[0] = config.vlanPorts()[6];
  config.vlanPorts()->resize(6);
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  auto vlansV2 = stateV2->getVlans();
  ASSERT_NE(nullptr, vlansV2);
  EXPECT_EQ(2, vlansV2->getGeneration());
  EXPECT_EQ(2, vlansV2->size());

  // VLAN 1234 should have changed
  auto vlan1234v1 = vlansV2->getVlan(VlanID(1234));
  EXPECT_NE(vlan1234v0, vlan1234v1);
  EXPECT_EQ(1, vlan1234v1->getGeneration());
  Vlan::MemberPorts ports1234v1;
  ports1234v1.insert(make_pair(2, false));
  ports1234v1.insert(make_pair(3, false));
  ports1234v1.insert(make_pair(4, false));
  EXPECT_EQ(ports1234v1, vlan1234v1->getPorts());

  // VLAN 99 should not have changed
  EXPECT_EQ(vlan99v0, vlansV2->getVlan(VlanID(99)));

  checkChangedVlans(vlansV1, vlansV2, {1234}, {}, {});
  EXPECT_EQ(id1234, vlansV2->getVlan(VlanID(1234))->getNodeID());
  EXPECT_EQ(id99, vlansV2->getVlan(VlanID(99))->getNodeID());

  // Remove VLAN 99
  config.vlans()->resize(1);
  *config.vlans()[0].id() = 1234;
  config.vlanPorts()->resize(3);
  *config.vlanPorts()[0].vlanID() = 1234;
  *config.vlanPorts()[0].logicalPort() = 2;
  *config.vlanPorts()[1].vlanID() = 1234;
  *config.vlanPorts()[1].logicalPort() = 3;
  *config.vlanPorts()[2].vlanID() = 1234;
  *config.vlanPorts()[2].logicalPort() = 4;
  config.interfaces()->resize(1);
  *config.interfaces()[0].intfID() = 1234;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto vlansV3 = stateV3->getVlans();
  ASSERT_NE(nullptr, vlansV3);
  EXPECT_EQ(3, vlansV3->getGeneration());
  EXPECT_EQ(1, vlansV3->size());

  // VLAN 1234 should not have changed
  EXPECT_EQ(vlan1234v1, vlansV3->getVlan(VlanID(1234)));
  // VLAN 99 should no longer exist
  EXPECT_EQ(nullptr, vlansV3->getVlanIf(VlanID(99)));

  checkChangedVlans(vlansV2, vlansV3, {}, {}, {99});
}
