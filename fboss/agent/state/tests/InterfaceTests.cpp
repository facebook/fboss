/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::MacAddress;
using std::make_shared;
using std::shared_ptr;
using ::testing::Return;

TEST(Interface, addrToReach) {
  MockPlatform platform;
  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;
  config.interfaces.resize(2);
  auto* intfConfig = &config.interfaces[0];
  intfConfig->intfID = 1;
  intfConfig->vlanID = 1;
  intfConfig->routerID = 1;
  intfConfig->mac = "00:02:00:11:22:33";
  intfConfig->__isset.mac = true;
  intfConfig->ipAddresses.resize(4);
  intfConfig->ipAddresses[0] = "10.1.1.1/24";
  intfConfig->ipAddresses[1] = "20.1.1.2/24";
  intfConfig->ipAddresses[2] = "::22:33:44/120";
  intfConfig->ipAddresses[3] = "::11:11:11/120";

  intfConfig = &config.interfaces[1];
  intfConfig->intfID = 2;
  intfConfig->vlanID = 2;
  intfConfig->routerID = 2;
  intfConfig->mac = "00:02:00:11:22:33";
  intfConfig->__isset.mac = true;
  intfConfig->ipAddresses.resize(4);
  intfConfig->ipAddresses[0] = "10.1.1.1/24";
  intfConfig->ipAddresses[1] = "20.1.1.2/24";
  intfConfig->ipAddresses[2] = "::22:33:44/120";
  intfConfig->ipAddresses[3] = "::11:11:11/120";

  InterfaceID id(1);
  shared_ptr<SwitchState> oldState = make_shared<SwitchState>();
  auto state = publishAndApplyConfig(oldState, &config, &platform);
  ASSERT_NE(nullptr, state);
  const auto& intfs = state->getInterfaces();
  const auto& intf1 = intfs->getInterface(InterfaceID(1));
  const auto& intf2 = intfs->getInterface(InterfaceID(2));

  EXPECT_TRUE(intf1->hasAddress(IPAddress("10.1.1.1")));
  EXPECT_FALSE(intf1->hasAddress(IPAddress("10.1.2.1")));
  EXPECT_TRUE(intf2->hasAddress(IPAddress("::11:11:11")));
  EXPECT_FALSE(intf2->hasAddress(IPAddress("::11:11:12")));

  auto ret = intfs->getIntfAddrToReach(RouterID(1), IPAddress("20.1.1.100"));
  EXPECT_EQ(intf1.get(), ret.intf);
  EXPECT_EQ(IPAddress("20.1.1.2"), *ret.addr);
  EXPECT_EQ(24, ret.mask);

  ret = intfs->getIntfAddrToReach(RouterID(2), IPAddress("::22:33:4f"));
  EXPECT_EQ(intf2.get(), ret.intf);
  EXPECT_EQ(IPAddress("::22:33:44"), *ret.addr);
  EXPECT_EQ(120, ret.mask);

  ret = intfs->getIntfAddrToReach(RouterID(2), IPAddress("::22:34:5f"));
  EXPECT_EQ(nullptr, ret.intf);
  EXPECT_EQ(nullptr, ret.addr);
  EXPECT_EQ(0, ret.mask);
}

TEST(Interface, applyConfig) {
  MockPlatform platform;
  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;
  config.interfaces.resize(1);
  auto* intfConfig = &config.interfaces[0];
  intfConfig->intfID = 1;
  intfConfig->vlanID = 1;
  intfConfig->routerID = 0;
  intfConfig->mac = "00:02:00:11:22:33";
  intfConfig->__isset.mac = true;

  InterfaceID id(1);
  shared_ptr<SwitchState> oldState;
  shared_ptr<SwitchState> state;
  shared_ptr<Interface> oldInterface;
  shared_ptr<Interface> interface;
  auto updateState = [&]() {
    oldState = state;
    oldInterface = interface;
    state = publishAndApplyConfig(oldState, &config, &platform);
    EXPECT_NE(oldState, state);
    ASSERT_NE(nullptr, state);
    interface = state->getInterfaces()->getInterface(id);
    EXPECT_NE(oldInterface, interface);
    ASSERT_NE(nullptr, interface);
  };

  state = make_shared<SwitchState>();
  updateState();
  NodeID nodeID = interface->getNodeID();
  EXPECT_EQ(0, interface->getGeneration());
  EXPECT_EQ(VlanID(1), interface->getVlanID());
  EXPECT_EQ(RouterID(0), interface->getRouterID());
  EXPECT_EQ("Interface 1", interface->getName());
  EXPECT_EQ(MacAddress("00:02:00:11:22:33"), interface->getMac());
  EXPECT_EQ(Interface::Addresses{}, interface->getAddresses());
  EXPECT_EQ(0, interface->getNdpConfig().routerAdvertisementSeconds);

  // same configuration cause nothing changed
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, &platform));

  // vlanID change
  intfConfig->vlanID = 2;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(0), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());

  // routerID change
  intfConfig->routerID = 1;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());

  // MAC address change
  intfConfig->mac = "00:02:00:12:34:56";
  updateState();
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(MacAddress("00:02:00:12:34:56"), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());
  // Use the platform supplied MAC
  intfConfig->mac = "";
  intfConfig->__isset.mac = false;
  MacAddress platformMac("00:02:00:ab:cd:ef");
  EXPECT_CALL(platform, getLocalMac()).WillRepeatedly(Return(platformMac));
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(platformMac, interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());

  // IP addresses change
  intfConfig->ipAddresses.resize(4);
  intfConfig->ipAddresses[0] = "10.1.1.1/24";
  intfConfig->ipAddresses[1] = "20.1.1.2/24";
  intfConfig->ipAddresses[2] = "::22:33:44/120";
  intfConfig->ipAddresses[3] = "::11:11:11/120";
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(4, interface->getAddresses().size());

  // change the order of IP address shall not change the interface
  intfConfig->ipAddresses[0] = "10.1.1.1/24";
  intfConfig->ipAddresses[1] = "::22:33:44/120";
  intfConfig->ipAddresses[2] = "20.1.1.2/24";
  intfConfig->ipAddresses[3] = "::11:11:11/120";
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, &platform));

  // duplicate IP addresses causes throw
  intfConfig->ipAddresses[1] = intfConfig->ipAddresses[0];
  EXPECT_THROW(publishAndApplyConfig(state, &config, &platform), FbossError);
  // Should still throw even if the mask is different
  intfConfig->ipAddresses[1] = "10.1.1.1/16";
  EXPECT_THROW(publishAndApplyConfig(state, &config, &platform), FbossError);
  intfConfig->ipAddresses[1] = "::22:33:44/120";

  // Name change
  intfConfig->name = "myintf";
  intfConfig->__isset.name = true;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ("myintf", interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());
  // Reset the name back to it's default value
  intfConfig->__isset.name = false;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ("Interface 1", interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());
  EXPECT_EQ(oldInterface->getNdpConfig(), interface->getNdpConfig());

  // Change the NDP configuration
  intfConfig->__isset.ndp = true;
  intfConfig->ndp.routerAdvertisementSeconds = 4;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(4, interface->getNdpConfig().routerAdvertisementSeconds);
  // Update the RA interval to 30 seconds
  intfConfig->ndp.routerAdvertisementSeconds = 30;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(30, interface->getNdpConfig().routerAdvertisementSeconds);
  // Drop the NDP configuration
  intfConfig->__isset.ndp = false;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(0, interface->getNdpConfig().routerAdvertisementSeconds);

  // Changing the ID creates a new interface
  intfConfig->intfID = 2;
  id = InterfaceID(2);
  updateState();
  // The generation number for the new interface will be 0
  EXPECT_NE(nodeID, interface->getNodeID());
  EXPECT_EQ(0, interface->getGeneration());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ("Interface 2", interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddresses(), interface->getAddresses());
}

/*
 * Test that forEachChanged(StateDelta::getIntfsDelta(), ...) invokes the
 * callback for the specified list of changed interfaces.
 */
void checkChangedIntfs(const shared_ptr<InterfaceMap>& oldIntfs,
                       const shared_ptr<InterfaceMap>& newIntfs,
                       const std::set<uint16_t> changedIDs,
                       const std::set<uint16_t> addedIDs,
                       const std::set<uint16_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetIntfs(oldIntfs);
  auto newState = make_shared<SwitchState>();
  newState->resetIntfs(newIntfs);

  std::set<uint16_t> foundChanged;
  std::set<uint16_t> foundAdded;
  std::set<uint16_t> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(delta.getIntfsDelta(),
    [&] (const shared_ptr<Interface>& oldIntf,
         const shared_ptr<Interface>& newIntf) {
      EXPECT_EQ(oldIntf->getID(), newIntf->getID());
      EXPECT_NE(oldIntf, newIntf);

      auto ret = foundChanged.insert(oldIntf->getID());
      EXPECT_TRUE(ret.second);
    },
    [&] (const shared_ptr<Interface>& intf) {
      auto ret = foundAdded.insert(intf->getID());
      EXPECT_TRUE(ret.second);
    },
    [&] (const shared_ptr<Interface>& intf) {
      auto ret = foundRemoved.insert(intf->getID());
      EXPECT_TRUE(ret.second);
    });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TEST(InterfaceMap, applyConfig) {
  MockPlatform platform;
  auto stateV0 = make_shared<SwitchState>();
  auto intfsV0 = stateV0->getInterfaces();

  cfg::SwitchConfig config;
  config.vlans.resize(3);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;
  config.vlans[2].id = 3;
  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  ASSERT_NE(nullptr, stateV1);
  auto intfsV1 = stateV1->getInterfaces();
  EXPECT_NE(intfsV1, intfsV0);
  EXPECT_EQ(1, intfsV1->getGeneration());
  EXPECT_EQ(2, intfsV1->size());

  // verify interface intfID==1
  auto intf1 = intfsV1->getInterface(InterfaceID(1));
  ASSERT_NE(nullptr, intf1);
  EXPECT_EQ(VlanID(1), intf1->getVlanID());
  EXPECT_EQ("00:00:00:00:00:11", intf1->getMac().toString());
  EXPECT_EQ(0, intf1->getGeneration());

  checkChangedIntfs(intfsV0, intfsV1, {}, {1, 2}, {});

  // getInterface() should throw on a non-existent interface
  EXPECT_THROW(intfsV1->getInterface(InterfaceID(99)), FbossError);
  // getInterfaceIf() should return nullptr on a non-existent interface
  EXPECT_EQ(nullptr, intfsV1->getInterfaceIf(InterfaceID(99)));

  // applying the same configure results in no change
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, &platform));

  // adding some IP addresses
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "192.168.1.1/16";
  config.interfaces[1].ipAddresses[1] = "::1/48";
  auto stateV2 = publishAndApplyConfig(stateV1, &config, &platform);
  ASSERT_NE(nullptr, stateV2);
  auto intfsV2 = stateV2->getInterfaces();
  EXPECT_NE(intfsV1, intfsV2);
  EXPECT_EQ(2, intfsV2->getGeneration());
  EXPECT_EQ(2, intfsV2->size());
  auto intf2 = intfsV2->getInterface(InterfaceID(2));
  EXPECT_EQ(2, intf2->getAddresses().size());

  checkChangedIntfs(intfsV1, intfsV2, {2}, {}, {});

  // add a new one together with deleting an existing one
  config.interfaces[0].intfID = 3;
  config.interfaces[0].vlanID = 3;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:33";
  auto stateV3 = publishAndApplyConfig(stateV2, &config, &platform);
  ASSERT_NE(nullptr, stateV3);
  auto intfsV3 = stateV3->getInterfaces();
  EXPECT_NE(intfsV2, intfsV3);
  EXPECT_EQ(3, intfsV3->getGeneration());
  EXPECT_EQ(2, intfsV3->size());
  auto intf3 = intfsV3->getInterface(InterfaceID(3));
  EXPECT_EQ(0, intf3->getAddresses().size());
  EXPECT_EQ(config.interfaces[0].mac, intf3->getMac().toString());
  // intf 1 should not be there anymroe
  EXPECT_EQ(nullptr, intfsV3->getInterfaceIf(InterfaceID(1)));

  checkChangedIntfs(intfsV2, intfsV3, {}, {3}, {1});

  // change the MTU
  config.interfaces[0].mtu = 1337;
  config.interfaces[0].__isset.mtu = true;
  EXPECT_EQ(1500, intfsV3->getInterface(InterfaceID(3))->getMtu());
  auto stateV4 = publishAndApplyConfig(stateV3, &config, &platform);
  ASSERT_NE(nullptr, stateV4);
  auto intfsV4 = stateV4->getInterfaces();
  EXPECT_NE(intfsV3, intfsV4);
  EXPECT_EQ(4, intfsV4->getGeneration());
  EXPECT_EQ(1337, intfsV4->getInterface(InterfaceID(3))->getMtu());
}
