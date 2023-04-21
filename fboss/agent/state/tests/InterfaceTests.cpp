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
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::MacAddress;
using std::make_shared;
using std::shared_ptr;
using ::testing::Return;

namespace {
void validateSerialization(const InterfaceMap& node) {
  auto nodeBack = std::make_shared<InterfaceMap>(node.toThrift());
  EXPECT_EQ(node.toThrift(), nodeBack->toThrift());
}
} // namespace

class InterfaceTest : public ::testing::Test {
 private:
  cfg::SwitchConfig genConfigWithLLs(
      std::set<std::string> intfLinkLocals,
      std::optional<std::string> raRouterAddr) {
    cfg::SwitchConfig config;
    config.vlans()->resize(2);
    *config.vlans()[0].id() = 1;
    *config.vlans()[1].id() = 2;
    config.interfaces()->resize(2);
    auto* intfConfig = &config.interfaces()[0];
    *intfConfig->intfID() = 1;
    *intfConfig->vlanID() = 1;
    *intfConfig->routerID() = 1;
    intfConfig->ipAddresses()->resize(4 + intfLinkLocals.size());
    auto idx = 0;
    intfConfig->ipAddresses()[idx++] = "10.1.1.1/24";
    intfConfig->ipAddresses()[idx++] = "20.1.1.2/24";
    intfConfig->ipAddresses()[idx++] = "::22:33:44/120";
    intfConfig->ipAddresses()[idx++] = "::11:11:11/120";
    for (const auto& intfAddr : intfLinkLocals) {
      intfConfig->ipAddresses()[idx++] = intfAddr;
    }
    if (raRouterAddr) {
      intfConfig->ndp() = cfg::NdpConfig{};
      intfConfig->ndp()->routerAddress() = *raRouterAddr;
    }

    intfConfig = &config.interfaces()[1];
    *intfConfig->intfID() = 2;
    *intfConfig->vlanID() = 2;
    *intfConfig->routerID() = 2;
    intfConfig->ipAddresses()->resize(4);
    intfConfig->ipAddresses()[0] = "10.1.1.1/24";
    intfConfig->ipAddresses()[1] = "20.1.1.2/24";
    intfConfig->ipAddresses()[2] = "::22:33:44/120";
    intfConfig->ipAddresses()[3] = "::11:11:11/120";
    return config;
  }

 protected:
  std::shared_ptr<SwitchState> setup(
      std::set<std::string> intfLinkLocals,
      std::optional<std::string> raRouterAddr) {
    platform_ = createMockPlatform();
    auto config = genConfigWithLLs(intfLinkLocals, raRouterAddr);
    return publishAndApplyConfig(
        std::make_shared<SwitchState>(), &config, platform_.get());
  }

 private:
  std::unique_ptr<Platform> platform_;
};

TEST_F(InterfaceTest, addrToReach) {
  auto state = setup({"fe80::face:b00c/64"}, std::nullopt);
  ASSERT_NE(nullptr, state);
  const auto& intfs = state->getInterfaces();
  const auto& intf1 = intfs->getInterface(InterfaceID(1));
  const auto& intf2 = intfs->getInterface(InterfaceID(2));

  validateThriftStructNodeSerialization(*intf1);
  validateThriftStructNodeSerialization(*intf2);

  EXPECT_TRUE(intf1->hasAddress(IPAddress("10.1.1.1")));
  EXPECT_FALSE(intf1->hasAddress(IPAddress("10.1.2.1")));
  EXPECT_TRUE(intf2->hasAddress(IPAddress("::11:11:11")));
  EXPECT_FALSE(intf2->hasAddress(IPAddress("::11:11:12")));

  auto intf = intfs->getIntfToReach(RouterID(1), IPAddress("20.1.1.100"));
  ASSERT_TRUE(intf != nullptr);
  EXPECT_EQ(intf1, intf);
  auto addr = intf->getAddressToReach(IPAddress("20.1.1.100"));
  EXPECT_EQ(IPAddress("20.1.1.2"), addr->first);
  EXPECT_EQ(24, addr->second);

  intf = intfs->getIntfToReach(RouterID(2), IPAddress("::22:33:4f"));
  ASSERT_TRUE(intf != nullptr);
  EXPECT_EQ(intf2, intf);
  addr = intf->getAddressToReach(IPAddress("::22:33:4f"));
  EXPECT_EQ(IPAddress("::22:33:44"), addr->first);
  EXPECT_EQ(120, addr->second);

  intf = intfs->getIntfToReach(RouterID(2), IPAddress("::22:34:5f"));
  ASSERT_TRUE(intf == nullptr);
  // Assert the to reach LL we always use fe80::face:b00c address
  // This is not required by NDP RFC (we could use any of the in
  // subnet addresses), but using fe80::face:b00c is done to
  // preserve longstanding behavior. See S289408 for details.
  EXPECT_EQ(
      IPAddress("fe80::face:b00c"),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendNw) {
  auto state =
      setup({"fe80::face:b00b/64", "fe80::be:face:b00c/64"}, std::nullopt);
  const auto& intf1 = state->getInterfaces()->getInterface(InterfaceID(1));
  EXPECT_EQ(
      IPAddress("fe80::face:b00b"),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendNwNewConfig) {
  auto platform = createMockPlatform();
  auto state = setup({"fe80::be:face:b00c/64"}, std::nullopt);
  const auto& intf1 = state->getInterfaces()->getInterface(InterfaceID(1));
  EXPECT_EQ(
      IPAddress("fe80::be:face:b00c"),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachWithRouterAddrConfigured) {
  auto state = setup({"fe80::face:b00c/64"}, "fe80::face:b00c");
  const auto& intf1 = state->getInterfaces()->getInterface(InterfaceID(1));
  EXPECT_EQ(
      MockPlatform::getMockLinkLocalIp6(),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendRouterAddrConfigured) {
  auto state =
      setup({"fe80::face:b00b/64", "fe80::be:face:b00c/64"}, "fe80::face:b00b");
  const auto& intf1 = state->getInterfaces()->getInterface(InterfaceID(1));
  EXPECT_EQ(
      MockPlatform::getMockLinkLocalIp6(),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendNewConfigRouterAddrConfigured) {
  auto state = setup({"fe80::be:face:b00c/64"}, "fe80::be:face:b00c");
  const auto& intf1 = state->getInterfaces()->getInterface(InterfaceID(1));
  EXPECT_EQ(
      MockPlatform::getMockLinkLocalIp6(),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, getSetArpTable) {
  auto state = setup({}, std::nullopt);
  state::NeighborEntries arpTable;
  state::NeighborEntryFields arp;
  arp.ipaddress() = "10.1.1.100";
  arp.mac() = "02:00:00:00:00:01";
  cfg::PortDescriptor port;
  port.portId() = 1;
  port.portType() = cfg::PortDescriptorType::Physical;
  arp.portId() = port;
  arp.interfaceId() = 1;
  arp.state() = state::NeighborState::Reachable;
  arpTable.insert({*arp.ipaddress(), arp});
  auto intf1 = state->getInterfaces()->getInterface(InterfaceID(1))->clone();
  intf1->setArpTable(arpTable);
  EXPECT_EQ(arpTable, intf1->getArpTable()->toThrift());
  EXPECT_EQ(
      arpTable, intf1->getNeighborEntryTable<folly::IPAddressV4>()->toThrift());
  EXPECT_NE(
      arpTable, intf1->getNeighborEntryTable<folly::IPAddressV6>()->toThrift());
}

TEST_F(InterfaceTest, getSetNdpTable) {
  auto state = setup({}, std::nullopt);
  state::NeighborEntries ndpTable;
  state::NeighborEntryFields ndp;
  ndp.ipaddress() = "::22:33:4f";
  ndp.mac() = "02:00:00:00:00:01";
  cfg::PortDescriptor port;
  port.portId() = 1;
  port.portType() = cfg::PortDescriptorType::Physical;
  ndp.portId() = port;
  ndp.interfaceId() = 1;
  ndp.state() = state::NeighborState::Reachable;
  ndpTable.insert({*ndp.ipaddress(), ndp});
  auto intf1 = state->getInterfaces()->getInterface(InterfaceID(1))->clone();
  intf1->setNdpTable(ndpTable);
  EXPECT_EQ(ndpTable, intf1->getNdpTable()->toThrift());
  EXPECT_EQ(
      ndpTable, intf1->getNeighborEntryTable<folly::IPAddressV6>()->toThrift());
  EXPECT_NE(
      ndpTable, intf1->getNeighborEntryTable<folly::IPAddressV4>()->toThrift());
}

TEST(Interface, Modify) {
  {
    auto state = std::make_shared<SwitchState>();
    auto origIntfs = state->getInterfaces();
    EXPECT_EQ(origIntfs.get(), origIntfs->modify(&state));
    state->publish();
    EXPECT_NE(origIntfs.get(), origIntfs->modify(&state));
    EXPECT_NE(origIntfs.get(), state->getInterfaces().get());
  }
  {
    // Remote sys ports modify
    auto state = std::make_shared<SwitchState>();
    auto origRemoteIntfs = state->getRemoteInterfaces();
    EXPECT_EQ(origRemoteIntfs.get(), origRemoteIntfs->modify(&state));
    state->publish();
    EXPECT_NE(origRemoteIntfs.get(), origRemoteIntfs->modify(&state));
    EXPECT_NE(origRemoteIntfs.get(), state->getRemoteInterfaces().get());
  }
}

TEST(Interface, applyConfig) {
  auto platform = createMockPlatform();
  cfg::SwitchConfig config;
  config.vlans()->resize(1);
  *config.vlans()[0].id() = 1;
  config.vlans()[0].intfID() = 1;
  config.interfaces()->resize(1);
  auto* intfConfig = &config.interfaces()[0];
  *intfConfig->intfID() = 1;
  *intfConfig->vlanID() = 1;
  *intfConfig->routerID() = 0;
  intfConfig->mac() = "00:02:00:11:22:33";

  InterfaceID id(1);
  shared_ptr<SwitchState> oldState;
  shared_ptr<SwitchState> state;
  shared_ptr<Interface> oldInterface;
  shared_ptr<Interface> interface;
  auto updateState = [&]() {
    oldState = state;
    oldInterface = interface;
    state = publishAndApplyConfig(oldState, &config, platform.get());
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
  EXPECT_EQ(1, interface->getAddresses()->size()); // 1 ipv6 link local address
  EXPECT_EQ(0, interface->routerAdvertisementSeconds());
  auto vlan1 = state->getVlans()->getVlanIf(VlanID(1));
  EXPECT_EQ(InterfaceID(1), vlan1->getInterfaceID());
  // same configuration cause nothing changed
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, platform.get()));

  // Change VlanID for intf + create new intf for existing vlan
  config.vlans()->resize(2);
  *config.vlans()[1].id() = 2;
  config.vlans()[1].intfID() = 1;
  *intfConfig->vlanID() = 2;
  config.interfaces()->resize(2);
  *config.interfaces()[1].intfID() = 5;
  *config.interfaces()[1].vlanID() = 1;
  *config.interfaces()[1].routerID() = 0;
  MacAddress intf2Mac("02:01:02:ab:cd:78");
  config.interfaces()[1].mac() = intf2Mac.toString();
  config.vlans()[0].intfID() = 5;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(0), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());
  auto vlan2 = state->getVlans()->getVlanIf(VlanID(2));
  auto newvlan1 = state->getVlans()->getVlanIf(VlanID(1));
  EXPECT_EQ(InterfaceID(1), vlan2->getInterfaceID());
  EXPECT_EQ(InterfaceID(5), newvlan1->getInterfaceID());

  // routerID change
  *config.interfaces()[0].routerID() = 1;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(InterfaceID(1), interface->getID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());

  // MAC address change
  config.interfaces()[0].mac() = "00:02:00:12:34:56";
  updateState();
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(MacAddress("00:02:00:12:34:56"), interface->getMac());
  // Use the platform supplied MAC
  config.interfaces()[0].mac().reset();
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(platform->getLocalMac(), interface->getMac());
  // Interface will be updated based on new MAC Address
  EXPECT_NE(oldInterface->getAddresses(), interface->getAddresses());

  // IP addresses change
  config.interfaces()[0].ipAddresses()->resize(4);
  config.interfaces()[0].ipAddresses()[0] = "10.1.1.1/24";
  config.interfaces()[0].ipAddresses()[1] = "20.1.1.2/24";
  config.interfaces()[0].ipAddresses()[2] = "::22:33:44/120";
  config.interfaces()[0].ipAddresses()[3] = "::11:11:11/120";
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  // Link-local addrs will be added automatically
  EXPECT_EQ(5, interface->getAddresses()->size());

  // change the order of IP address shall not change the interface
  config.interfaces()[0].ipAddresses()[0] = "10.1.1.1/24";
  config.interfaces()[0].ipAddresses()[1] = "::22:33:44/120";
  config.interfaces()[0].ipAddresses()[2] = "20.1.1.2/24";
  config.interfaces()[0].ipAddresses()[3] = "::11:11:11/120";
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, platform.get()));

  // duplicate IP addresses causes throw
  config.interfaces()[0].ipAddresses()[1] =
      config.interfaces()[0].ipAddresses()[0];
  EXPECT_THROW(
      publishAndApplyConfig(state, &config, platform.get()), FbossError);
  // Should still throw even if the mask is different
  config.interfaces()[0].ipAddresses()[1] = "10.1.1.1/16";
  EXPECT_THROW(
      publishAndApplyConfig(state, &config, platform.get()), FbossError);
  config.interfaces()[0].ipAddresses()[1] = "::22:33:44/120";

  // Name change
  config.interfaces()[0].name() = "myintf";
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ("myintf", interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());
  // Reset the name back to it's default value
  config.interfaces()[0].name().reset();
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ("Interface 1", interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());
  EXPECT_EQ(
      oldInterface->getNdpConfig()->toThrift(),
      interface->getNdpConfig()->toThrift());

  // Change the NDP configuration
  config.interfaces()[0].ndp() = cfg::NdpConfig();
  *config.interfaces()[0].ndp()->routerAdvertisementSeconds() = 4;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(
      oldInterface->getAddresses()->toThrift(),
      interface->getAddresses()->toThrift());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(4, interface->routerAdvertisementSeconds());
  // Update the RA interval to 30 seconds
  *config.interfaces()[0].ndp()->routerAdvertisementSeconds() = 30;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(30, interface->routerAdvertisementSeconds());
  // Drop the NDP configuration
  config.interfaces()[0].ndp().reset();
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(0, interface->routerAdvertisementSeconds());

  // Changing the ID creates a new interface
  *config.interfaces()[0].intfID() = 2;
  id = InterfaceID(2);
  updateState();
  // The generation number for the new interface will be 0
  EXPECT_NE(nodeID, interface->getNodeID());
  EXPECT_EQ(0, interface->getGeneration());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ("Interface 2", interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());
  validateThriftStructNodeSerialization(*interface);
}

/*
 * Test that forEachChanged(StateDelta::getIntfsDelta(), ...) invokes the
 * callback for the specified list of changed interfaces.
 */
void checkChangedIntfs(
    const shared_ptr<InterfaceMap>& oldIntfs,
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
  DeltaFunctions::forEachChanged(
      delta.getIntfsDelta(),
      [&](const shared_ptr<Interface>& oldIntf,
          const shared_ptr<Interface>& newIntf) {
        EXPECT_EQ(oldIntf->getID(), newIntf->getID());
        EXPECT_NE(oldIntf, newIntf);

        auto ret = foundChanged.insert(oldIntf->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<Interface>& intf) {
        auto ret = foundAdded.insert(intf->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<Interface>& intf) {
        auto ret = foundRemoved.insert(intf->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);

  validateSerialization(*oldIntfs);
  validateSerialization(*newIntfs);
}

TEST(InterfaceMap, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto intfsV0 = stateV0->getInterfaces();

  cfg::SwitchConfig config;
  config.vlans()->resize(2);
  *config.vlans()[0].id() = 1;
  config.vlans()[0].intfID() = 1;
  *config.vlans()[1].id() = 2;
  config.vlans()[1].intfID() = 2;
  config.interfaces()->resize(2);
  *config.interfaces()[0].intfID() = 1;
  *config.interfaces()[0].vlanID() = 1;
  config.interfaces()[0].mac() = "00:00:00:00:00:11";
  *config.interfaces()[1].intfID() = 2;
  *config.interfaces()[1].vlanID() = 2;
  config.interfaces()[1].mac() = "00:00:00:00:00:22";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intfsV1 = stateV1->getInterfaces();
  EXPECT_NE(intfsV1, intfsV0);
  EXPECT_EQ(1, intfsV1->getGeneration());
  EXPECT_EQ(2, intfsV1->size());

  // verify interface intfID==1
  auto intf1 = intfsV1->getInterface(InterfaceID(1));
  auto vlan1 = stateV1->getVlans()->getVlanIf(intf1->getVlanID());
  ASSERT_NE(nullptr, intf1);
  EXPECT_EQ(VlanID(1), intf1->getVlanID());
  EXPECT_EQ("00:00:00:00:00:11", intf1->getMac().toString());
  EXPECT_EQ(0, intf1->getGeneration());
  EXPECT_EQ(vlan1->getInterfaceID(), intf1->getID());
  checkChangedIntfs(intfsV0, intfsV1, {}, {1, 2}, {});

  // getInterface() should throw on a non-existent interface
  EXPECT_THROW(intfsV1->getInterface(InterfaceID(99)), FbossError);
  // getInterfaceIf() should return nullptr on a non-existent interface
  EXPECT_EQ(nullptr, intfsV1->getInterfaceIf(InterfaceID(99)));

  // applying the same configure results in no change
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, platform.get()));

  // adding some IP addresses
  config.interfaces()[1].ipAddresses()->resize(2);
  config.interfaces()[1].ipAddresses()[0] = "192.168.1.1/16";
  config.interfaces()[1].ipAddresses()[1] = "::1/48";
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  auto intfsV2 = stateV2->getInterfaces();
  EXPECT_NE(intfsV1, intfsV2);
  EXPECT_EQ(2, intfsV2->getGeneration());
  EXPECT_EQ(2, intfsV2->size());
  auto intf2 = intfsV2->getInterface(InterfaceID(2));
  EXPECT_EQ(3, intf2->getAddresses()->size()); // v6 link-local is added

  checkChangedIntfs(intfsV1, intfsV2, {2}, {}, {});

  // add two new interfaces together with deleting an existing one
  config.vlans()->resize(3);
  *config.vlans()[2].id() = 3;
  config.vlans()[2].intfID() = 3;
  *config.interfaces()[0].intfID() = 3;
  *config.interfaces()[0].vlanID() = 3;
  config.interfaces()[0].mac() = "00:00:00:00:00:33";
  config.interfaces()->resize(3);
  *config.interfaces()[2].intfID() = 5;
  *config.interfaces()[2].vlanID() = 1;
  config.interfaces()[2].mac() = "00:00:00:00:00:55";
  config.vlans()[0].intfID() = 5;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  auto intfsV3 = stateV3->getInterfaces();
  EXPECT_NE(intfsV2, intfsV3);
  EXPECT_EQ(3, intfsV3->getGeneration());
  EXPECT_EQ(3, intfsV3->size());
  auto intf3 = intfsV3->getInterface(InterfaceID(3));
  EXPECT_EQ(1, intf3->getAddresses()->size());
  EXPECT_EQ(
      config.interfaces()[0].mac().value_or({}), intf3->getMac().toString());
  // intf 1 should not be there anymroe
  EXPECT_EQ(nullptr, intfsV3->getInterfaceIf(InterfaceID(1)));
  auto vlan3 = stateV3->getVlans()->getVlanIf(intf3->getVlanID());
  EXPECT_EQ(vlan3->getInterfaceID(), intf3->getID());
  auto newvlan1 = stateV3->getVlans()->getVlanIf(VlanID(1));
  EXPECT_EQ(InterfaceID(5), newvlan1->getInterfaceID());

  checkChangedIntfs(intfsV2, intfsV3, {}, {3, 5}, {1});

  // change the MTU
  config.interfaces()[0].mtu() = 1337;
  EXPECT_EQ(1500, intfsV3->getInterface(InterfaceID(3))->getMtu());
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  auto intfsV4 = stateV4->getInterfaces();
  EXPECT_NE(intfsV3, intfsV4);
  EXPECT_EQ(4, intfsV4->getGeneration());
  EXPECT_EQ(1337, intfsV4->getInterface(InterfaceID(3))->getMtu());
}

TEST(Interface, getLocalInterfacesBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto mySwitchId = stateV1->getSwitchSettings()->getSwitchId();
  CHECK(mySwitchId) << "Switch ID must be set for VOQ switch";
  auto myRif = stateV1->getInterfaces(SwitchID(*mySwitchId));
  EXPECT_EQ(myRif->size(), stateV1->getInterfaces()->size());
  // No remote sys ports
  EXPECT_EQ(stateV1->getInterfaces(SwitchID(*mySwitchId + 1))->size(), 0);
}

TEST(Interface, getRemoteInterfacesBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto mySwitchId = stateV1->getSwitchSettings()->getSwitchId();
  CHECK(mySwitchId) << "Switch ID must be set for VOQ switch";
  auto myRif = stateV1->getInterfaces(SwitchID(*mySwitchId));
  EXPECT_EQ(myRif->size(), stateV1->getInterfaces()->size());
  int64_t remoteSwitchId = 100;
  auto sysPort1 = makeSysPort("olympic", 1001, remoteSwitchId);
  auto stateV2 = stateV1->clone();
  auto remoteSysPorts = stateV2->getRemoteSystemPorts()->modify(&stateV2);
  remoteSysPorts->addSystemPort(sysPort1);
  auto remoteInterfaces = stateV2->getRemoteInterfaces()->modify(&stateV2);
  auto rif = std::make_shared<Interface>(
      InterfaceID(1001),
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("1001"),
      folly::MacAddress{},
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);

  remoteInterfaces->addInterface(rif);

  EXPECT_EQ(stateV2->getInterfaces(SwitchID(remoteSwitchId))->size(), 1);
}

TEST(Interface, getInterfaceSysPortIDVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_TRUE(intf->getSystemPortID().has_value());
  EXPECT_EQ(
      static_cast<int64_t>(intf->getID()),
      static_cast<int64_t>(intf->getSystemPortID().value()));
}

TEST(Interface, getInterfaceSysPortID) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_FALSE(intf->getSystemPortID().has_value());
}

TEST(Interface, getInterfaceSysPortRangeVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_TRUE(
      stateV1->getAssociatedSystemPortRangeIf(intf->getID()).has_value());
}

TEST(Interface, getInterfaceSysPortRange) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_FALSE(
      stateV1->getAssociatedSystemPortRangeIf(intf->getID()).has_value());
}

TEST(Interface, getInterfacePortsVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_EQ(getPortsForInterface(intf->getID(), stateV1).size(), 1);
}

TEST(Interface, getInterfacePorts) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  EXPECT_EQ(getPortsForInterface(intf->getID(), stateV1).size(), 11);
}

TEST(Interface, verifyPseudoVlanProcessing) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  auto verifyConfigPseudoVlansMatch = [](const auto& config,
                                         const auto& state) {
    for (const auto& interfaceCfg : *config.interfaces()) {
      for (const auto& addr : *interfaceCfg.ipAddresses()) {
        auto ipAddr = folly::IPAddress::createNetwork(addr, -1, false).first;
        auto expectedMac = interfaceCfg.mac();
        auto expectedIntfID = interfaceCfg.intfID();

        if (ipAddr.isV4()) {
          auto arpResponseEntry = state->getVlans()
                                      ->getVlan(VlanID(0))
                                      ->getArpResponseTable()
                                      ->getEntry(ipAddr.asV4());
          EXPECT_TRUE(arpResponseEntry != nullptr);
          // no MAC for recycle port RIFs
          if (expectedMac) {
            EXPECT_EQ(*expectedMac, arpResponseEntry->getMac().toString());
          }
          EXPECT_EQ(
              InterfaceID(*expectedIntfID), arpResponseEntry->getInterfaceID());
        } else {
          auto ndpResponseEntry = state->getVlans()
                                      ->getVlan(VlanID(0))
                                      ->getNdpResponseTable()
                                      ->getEntry(ipAddr.asV6());
          EXPECT_TRUE(ndpResponseEntry != nullptr);
          // no MAC for recycle port RIFs
          if (expectedMac) {
            EXPECT_EQ(*expectedMac, ndpResponseEntry->getMac().toString());
          }
          EXPECT_EQ(
              InterfaceID(*expectedIntfID), ndpResponseEntry->getInterfaceID());
        }
      }
    }
  };

  // Verify if pseudo vlans are populated correctly
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  verifyConfigPseudoVlansMatch(config, stateV1);

  // Apply same config, and verify no change in pseudo vlans
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(nullptr, stateV2);

  // Apply modified config (2 interfaces => 1 interface), and verify if pseudo
  // vlans are populated correctly
  auto config2 = testConfigA(cfg::SwitchType::VOQ);
  config2.interfaces()->resize(1);
  auto stateV3 = publishAndApplyConfig(stateV1, &config2, platform.get());
  verifyConfigPseudoVlansMatch(config2, stateV3);

  // Modify an interface (e.g. MAC addr for an interface), and verify if pseudo
  // vlans are populated correctly
  auto config3 = testConfigA(cfg::SwitchType::VOQ);
  auto currMac = folly::MacAddress(*config3.interfaces()[0].mac());
  auto newMac = folly::MacAddress::fromHBO(currMac.u64HBO() + 1);
  config3.interfaces()[0].mac() = newMac.toString();
  auto stateV4 = publishAndApplyConfig(stateV1, &config3, platform.get());
  verifyConfigPseudoVlansMatch(config3, stateV4);
}

TEST(Interface, modify) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto intf = stateV1->getInterfaces()->begin()->second;
  auto intfModified = intf->modify(&stateV1);
  EXPECT_EQ(intf.get(), intfModified);
  intf->publish();
  intfModified = intf->modify(&stateV1);
  EXPECT_NE(intf.get(), intfModified);
  auto oldMtu = intfModified->getMtu();
  auto newMtu = oldMtu + 1000;
  intfModified->setMtu(newMtu);
  EXPECT_EQ(
      stateV1->getInterfaces()->getInterface(intf->getID())->getMtu(), newMtu);
}
