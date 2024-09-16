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
#include "fboss/agent/state/StateUtils.h"
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
void validateSerialization(const std::shared_ptr<InterfaceMap>& node) {
  if (!node) {
    return;
  }
  auto nodeBack = std::make_shared<InterfaceMap>(node->toThrift());
  EXPECT_EQ(node->toThrift(), nodeBack->toThrift());
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
  const auto& intf1 = intfs->getNode(InterfaceID(1));
  const auto& intf2 = intfs->getNode(InterfaceID(2));

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
  const auto& intf1 = state->getInterfaces()->getNode(InterfaceID(1));
  EXPECT_EQ(
      IPAddress("fe80::face:b00b"),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendNwNewConfig) {
  auto platform = createMockPlatform();
  auto state = setup({"fe80::be:face:b00c/64"}, std::nullopt);
  const auto& intf1 = state->getInterfaces()->getNode(InterfaceID(1));
  EXPECT_EQ(
      IPAddress("fe80::be:face:b00c"),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachWithRouterAddrConfigured) {
  auto state = setup({"fe80::face:b00c/64"}, "fe80::face:b00c");
  const auto& intf1 = state->getInterfaces()->getNode(InterfaceID(1));
  EXPECT_EQ(
      MockPlatform::getMockLinkLocalIp6(),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendRouterAddrConfigured) {
  auto state =
      setup({"fe80::face:b00b/64", "fe80::be:face:b00c/64"}, "fe80::face:b00b");
  const auto& intf1 = state->getInterfaces()->getNode(InterfaceID(1));
  EXPECT_EQ(
      MockPlatform::getMockLinkLocalIp6(),
      intf1->getAddressToReach(IPAddress("fe80::9a03:9bff:fe7d:656a"))->first);
}

TEST_F(InterfaceTest, addrToReachBackendNewConfigRouterAddrConfigured) {
  auto state = setup({"fe80::be:face:b00c/64"}, "fe80::be:face:b00c");
  const auto& intf1 = state->getInterfaces()->getNode(InterfaceID(1));
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
  auto intf1 = state->getInterfaces()->getNode(InterfaceID(1))->clone();
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
  auto intf1 = state->getInterfaces()->getNode(InterfaceID(1))->clone();
  intf1->setNdpTable(ndpTable);
  EXPECT_EQ(ndpTable, intf1->getNdpTable()->toThrift());
  EXPECT_EQ(
      ndpTable, intf1->getNeighborEntryTable<folly::IPAddressV6>()->toThrift());
  EXPECT_NE(
      ndpTable, intf1->getNeighborEntryTable<folly::IPAddressV4>()->toThrift());
}

TEST(Interface, Modify) {
  {
    // NPU
    auto state = std::make_shared<SwitchState>();
    auto platform = createMockPlatform();
    cfg::SwitchConfig config = testConfigA();
    auto stateV1 = publishAndApplyConfig(state, &config, platform.get());
    stateV1->publish();
    auto origIntfMap = utility::getFirstMap(stateV1->getInterfaces());
    auto origIntf = origIntfMap->cbegin()->second;
    auto origIntfs = stateV1->getInterfaces();
    auto newIntf = origIntf->modify(&stateV1);
    EXPECT_NE(origIntf.get(), newIntf);
    EXPECT_NE(origIntfMap, utility::getFirstMap(stateV1->getInterfaces()));
    EXPECT_NE(origIntfs, stateV1->getInterfaces());
  }
  {
    // VOQ
    auto state = std::make_shared<SwitchState>();
    addSwitchInfo(state, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
    auto platform = createMockPlatform();
    cfg::SwitchConfig config = testConfigA(cfg::SwitchType::VOQ);
    auto stateV1 = publishAndApplyConfig(state, &config, platform.get());
    stateV1->publish();
    auto origIntfMap = utility::getFirstMap(stateV1->getInterfaces());
    auto origIntf = origIntfMap->cbegin()->second;
    auto origIntfs = stateV1->getInterfaces();
    auto newIntf = origIntf->modify(&stateV1);
    EXPECT_NE(origIntf.get(), newIntf);
    EXPECT_NE(origIntfMap, utility::getFirstMap(stateV1->getInterfaces()));
    EXPECT_NE(origIntfs, stateV1->getInterfaces());
  }
}

TEST(Interface, RemoteInterfaceModify) {
  auto state = std::make_shared<SwitchState>();
  auto platform = createMockPlatform();
  addSwitchInfo(state, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  cfg::SwitchConfig config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(state, &config, platform.get());
  auto remoteSysPorts = stateV1->getRemoteSystemPorts()->modify(&stateV1);

  HwSwitchMatcher scope(std::unordered_set<SwitchID>({SwitchID{1}}));
  auto sysPort1 = makeSysPort("olympic", 1001, 100);
  remoteSysPorts->addNode(sysPort1, scope);
  auto remoteInterfaces = stateV1->getRemoteInterfaces()->modify(&stateV1);
  InterfaceID kIntf(1001);
  auto rif = std::make_shared<Interface>(
      kIntf,
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("1001"),
      folly::MacAddress{},
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);

  remoteInterfaces->addNode(rif, scope);
  stateV1->publish();
  auto origIntfMap = stateV1->getRemoteInterfaces()->getMapNodeIf(scope);
  auto origIntf = stateV1->getRemoteInterfaces()->getNode(kIntf);
  auto origIntfs = stateV1->getRemoteInterfaces();
  auto newIntf = origIntf->modify(&stateV1);
  EXPECT_NE(origIntf.get(), newIntf);
  EXPECT_NE(origIntfMap, stateV1->getRemoteInterfaces()->getMapNodeIf(scope));
  EXPECT_NE(origIntfs, stateV1->getRemoteInterfaces());
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
  intfConfig->dhcpRelayAddressV4() = "30.1.1.1";
  intfConfig->dhcpRelayAddressV6() = "2a03:2880:10:1f07:face:b00c:0:0";
  intfConfig->dhcpRelayOverridesV4() = {};
  (*intfConfig->dhcpRelayOverridesV4())["02:00:00:00:00:02"] = "1.2.3.4";
  intfConfig->dhcpRelayOverridesV6() = {};
  (*intfConfig->dhcpRelayOverridesV6())["02:00:00:00:00:02"] =
      "2a03:2880:10:1f07:face:b00c:0:0";

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
    interface = state->getInterfaces()->getNode(id);
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
  auto vlan1 = state->getVlans()->getNodeIf(VlanID(1));
  EXPECT_EQ(InterfaceID(1), vlan1->getInterfaceID());
  EXPECT_EQ(folly::IPAddressV4("30.1.1.1"), interface->getDhcpV4Relay());
  EXPECT_EQ(
      folly::IPAddressV6("2a03:2880:10:1f07:face:b00c:0:0"),
      interface->getDhcpV6Relay());

  auto map4 = interface->getDhcpV4RelayOverrides();
  EXPECT_EQ(
      folly::IPAddressV4("1.2.3.4"),
      folly::IPAddressV4(map4[folly::MacAddress("02:00:00:00:00:02")]));
  auto map6 = interface->getDhcpV6RelayOverrides();
  EXPECT_EQ(
      folly::IPAddressV6("2a03:2880:10:1f07:face:b00c:0:0"),
      folly::IPAddressV6(map6[folly::MacAddress("02:00:00:00:00:02")]));

  // same configuration cause nothing changed
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, platform.get()));

  // Change VlanID for intf + create new intf for existing vlan
  config.vlans()->resize(2);
  *config.vlans()[1].id() = 2;
  config.vlans()[1].intfID() = 2;
  *intfConfig->vlanID() = 2;
  config.interfaces()->resize(2);
  *config.interfaces()[0].intfID() = 2;
  *config.interfaces()[0].vlanID() = 2;
  *config.interfaces()[1].intfID() = 1;
  *config.interfaces()[1].vlanID() = 1;
  *config.interfaces()[1].routerID() = 0;
  MacAddress intf2Mac("02:01:02:ab:cd:78");
  config.interfaces()[1].mac() = intf2Mac.toString();
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(1, interface->getGeneration());
  EXPECT_EQ(VlanID(1), interface->getVlanID());
  EXPECT_EQ(RouterID(0), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(intf2Mac, interface->getMac());
  auto vlan2 = state->getVlans()->getNodeIf(VlanID(2));
  auto newvlan1 = state->getVlans()->getNodeIf(VlanID(1));
  EXPECT_EQ(InterfaceID(2), vlan2->getInterfaceID());
  EXPECT_EQ(InterfaceID(1), newvlan1->getInterfaceID());

  // routerID change
  *config.interfaces()[1].routerID() = 1;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(1), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(InterfaceID(1), interface->getID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());

  // MAC address change
  config.interfaces()[1].mac() = "00:02:00:12:34:56";
  updateState();
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(1), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(MacAddress("00:02:00:12:34:56"), interface->getMac());
  // Use the platform supplied MAC
  config.interfaces()[1].mac().reset();
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
  config.interfaces()[1].ipAddresses()->resize(4);
  config.interfaces()[1].ipAddresses()[0] = "10.1.1.1/24";
  config.interfaces()[1].ipAddresses()[1] = "20.1.1.2/24";
  config.interfaces()[1].ipAddresses()[2] = "::22:33:44/120";
  config.interfaces()[1].ipAddresses()[3] = "::11:11:11/120";
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ(VlanID(1), interface->getVlanID());
  EXPECT_EQ(RouterID(1), interface->getRouterID());
  EXPECT_EQ(oldInterface->getName(), interface->getName());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  // Link-local addrs will be added automatically
  EXPECT_EQ(5, interface->getAddresses()->size());

  // change the order of IP address shall not change the interface
  config.interfaces()[1].ipAddresses()[0] = "10.1.1.1/24";
  config.interfaces()[1].ipAddresses()[1] = "::22:33:44/120";
  config.interfaces()[1].ipAddresses()[2] = "20.1.1.2/24";
  config.interfaces()[1].ipAddresses()[3] = "::11:11:11/120";
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, platform.get()));

  // duplicate IP addresses causes throw
  config.interfaces()[1].ipAddresses()[1] =
      config.interfaces()[1].ipAddresses()[0];
  EXPECT_THROW(
      publishAndApplyConfig(state, &config, platform.get()), FbossError);
  // Should still throw even if the mask is different
  config.interfaces()[1].ipAddresses()[1] = "10.1.1.1/16";
  EXPECT_THROW(
      publishAndApplyConfig(state, &config, platform.get()), FbossError);
  config.interfaces()[1].ipAddresses()[1] = "::22:33:44/120";

  // Name change
  config.interfaces()[1].name() = "myintf";
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_EQ("myintf", interface->getName());
  EXPECT_EQ(oldInterface->getVlanID(), interface->getVlanID());
  EXPECT_EQ(oldInterface->getRouterID(), interface->getRouterID());
  EXPECT_EQ(oldInterface->getMac(), interface->getMac());
  EXPECT_EQ(oldInterface->getAddressesCopy(), interface->getAddressesCopy());
  // Reset the name back to it's default value
  config.interfaces()[1].name().reset();
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
  config.interfaces()[1].ndp() = cfg::NdpConfig();
  *config.interfaces()[1].ndp()->routerAdvertisementSeconds() = 4;
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
  // Update the RA interval to 31 seconds
  *config.interfaces()[1].ndp()->routerAdvertisementSeconds() = 30;
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(30, interface->routerAdvertisementSeconds());
  // Drop the NDP configuration
  config.interfaces()[1].ndp().reset();
  updateState();
  EXPECT_EQ(nodeID, interface->getNodeID());
  EXPECT_EQ(oldInterface->getGeneration() + 1, interface->getGeneration());
  EXPECT_NE(oldInterface->getNdpConfig(), interface->getNdpConfig());
  EXPECT_EQ(0, interface->routerAdvertisementSeconds());

  // Change DHCP relay configuration
  config.interfaces()[1].dhcpRelayAddressV4() = "30.1.1.2";
  config.interfaces()[1].dhcpRelayAddressV6() =
      "2a03:2880:10:1f07:face:b00c:0:2";
  updateState();
  EXPECT_EQ(folly::IPAddressV4("30.1.1.2"), interface->getDhcpV4Relay());
  EXPECT_EQ(
      folly::IPAddressV6("2a03:2880:10:1f07:face:b00c:0:2"),
      interface->getDhcpV6Relay());

  // Change DHCP relay override configuration
  config.interfaces()[1].dhcpRelayOverridesV4() = {};
  (*config.interfaces()[1].dhcpRelayOverridesV4())["02:00:00:00:00:02"] =
      "1.2.3.5";
  config.interfaces()[1].dhcpRelayOverridesV6() = {};
  (*config.interfaces()[1].dhcpRelayOverridesV6())["02:00:00:00:00:02"] =
      "2a03:2880:10:1f07:face:b00c:0:2";
  updateState();

  auto map44 = interface->getDhcpV4RelayOverrides();
  EXPECT_EQ(
      folly::IPAddressV4("1.2.3.5"),
      folly::IPAddressV4(map44[folly::MacAddress("02:00:00:00:00:02")]));
  auto map66 = interface->getDhcpV6RelayOverrides();
  EXPECT_EQ(
      folly::IPAddressV6("2a03:2880:10:1f07:face:b00c:0:2"),
      folly::IPAddressV6(map66[folly::MacAddress("02:00:00:00:00:02")]));

  // Changing the ID creates a new interface
  *config.interfaces()[0].intfID() = 2;
  config.interfaces()[0].name() = "newName";
  id = InterfaceID(2);
  updateState();
  // The generation number for the new interface will be 0
  EXPECT_NE(nodeID, interface->getNodeID());
  EXPECT_EQ(VlanID(2), interface->getVlanID());
  EXPECT_EQ(RouterID(0), interface->getRouterID());
  EXPECT_EQ("newName", interface->getName());
  EXPECT_EQ(MacAddress("00:02:00:11:22:33"), interface->getMac());
  validateThriftStructNodeSerialization(*interface);
}

/*
 * Test that forEachChanged(StateDelta::getIntfsDelta(), ...) invokes the
 * callback for the specified list of changed interfaces.
 */
void checkChangedIntfs(
    const shared_ptr<MultiSwitchInterfaceMap>& oldIntfs,
    const shared_ptr<MultiSwitchInterfaceMap>& newIntfs,
    const std::set<uint16_t> changedIDs,
    const std::set<uint16_t> addedIDs,
    const std::set<uint16_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  auto newState = make_shared<SwitchState>();
  oldState->resetIntfs(oldIntfs);
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

  validateSerialization(utility::getFirstMap(oldIntfs));
  validateSerialization(utility::getFirstMap(newIntfs));
}

TEST(InterfaceMap, Modify) {
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
  EXPECT_EQ(2, intfsV1->numNodes());

  // verify interface intfID==1
  auto intf1 = intfsV1->getNode(InterfaceID(1));
  auto vlan1 = stateV1->getVlans()->getNodeIf(intf1->getVlanID());
  ASSERT_NE(nullptr, intf1);
  EXPECT_EQ(VlanID(1), intf1->getVlanID());
  EXPECT_EQ("00:00:00:00:00:11", intf1->getMac().toString());
  EXPECT_EQ(vlan1->getInterfaceID(), intf1->getID());
  checkChangedIntfs(intfsV0, intfsV1, {}, {1, 2}, {});

  // getInterface() should throw on a non-existent interface
  EXPECT_THROW(intfsV1->getNode(InterfaceID(99)), FbossError);
  // getInterfaceIf() should return nullptr on a non-existent interface
  EXPECT_EQ(nullptr, intfsV1->getNodeIf(InterfaceID(99)));

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
  EXPECT_EQ(2, intfsV2->numNodes());
  auto intf2 = intfsV2->getNode(InterfaceID(2));
  EXPECT_EQ(3, intf2->getAddresses()->size()); // v6 link-local is added

  checkChangedIntfs(intfsV1, intfsV2, {2}, {}, {});

  // add a new interface and change 1
  config.vlans()->resize(3);
  *config.vlans()[2].id() = 3;
  config.vlans()[2].intfID() = 3;
  config.interfaces()[0].mac() = "00:00:00:00:00:33";
  config.interfaces()->resize(3);
  *config.interfaces()[2].intfID() = 3;
  *config.interfaces()[2].vlanID() = 3;
  config.interfaces()[2].mac() = "00:00:00:00:00:55";
  config.vlans()[2].intfID() = 3;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  auto intfsV3 = stateV3->getInterfaces();
  EXPECT_NE(intfsV2, intfsV3);
  EXPECT_EQ(3, intfsV3->numNodes());
  auto intf3 = intfsV3->getNode(InterfaceID(3));
  EXPECT_EQ(1, intf3->getAddresses()->size());
  EXPECT_EQ(
      config.interfaces()[2].mac().value_or({}), intf3->getMac().toString());
  // intf 1 should not be there anymroe
  auto vlan3 = stateV3->getVlans()->getNodeIf(intf3->getVlanID());
  EXPECT_EQ(vlan3->getInterfaceID(), intf3->getID());
  auto newvlan1 = stateV3->getVlans()->getNodeIf(VlanID(1));

  checkChangedIntfs(intfsV2, intfsV3, {1}, {3}, {});

  // change the MTU
  config.interfaces()[0].mtu() = 1337;
  EXPECT_EQ(1500, intfsV3->getNode(InterfaceID(3))->getMtu());
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  auto intfsV4 = stateV4->getInterfaces();
  EXPECT_NE(intfsV3, intfsV4);
  EXPECT_EQ(1337, intfsV4->getNode(InterfaceID(1))->getMtu());
}

TEST(Interface, getLocalInterfacesBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto localSwitchId = kVoqSwitchIdBegin;
  auto myRif = stateV1->getInterfaces(SwitchID(localSwitchId));
  EXPECT_EQ(myRif->size(), stateV1->getInterfaces()->numNodes());
  // No remote sys ports
  EXPECT_EQ(stateV1->getInterfaces(SwitchID(localSwitchId + 1))->size(), 0);
}

TEST(Interface, getRemoteInterfacesBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto localSwitchId = kVoqSwitchIdBegin;
  auto myRif = stateV1->getInterfaces(SwitchID(localSwitchId));
  EXPECT_EQ(myRif->size(), stateV1->getInterfaces()->numNodes());
  int64_t remoteSwitchId = 100;
  auto sysPort1 = makeSysPort("olympic", 1001, remoteSwitchId);
  auto stateV2 = stateV1->clone();
  auto remoteSysPorts = stateV2->getRemoteSystemPorts()->modify(&stateV2);
  remoteSysPorts->addNode(
      sysPort1,
      HwSwitchMatcher(
          std::unordered_set<SwitchID>({SwitchID{kVoqSwitchIdBegin}})));
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

  remoteInterfaces->addNode(
      rif,
      HwSwitchMatcher(
          std::unordered_set<SwitchID>({SwitchID{remoteSwitchId}})));

  EXPECT_EQ(stateV2->getInterfaces(SwitchID(remoteSwitchId))->size(), 1);
}

TEST(Interface, getInterfaceSysPortIDVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
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
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  EXPECT_FALSE(intf->getSystemPortID().has_value());
}

TEST(Interface, getInterfaceSysPortRangeVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  EXPECT_TRUE(
      stateV1->getAssociatedSystemPortRangeIf(intf->getID()).has_value());
}

TEST(Interface, getInterfaceSysPortRange) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  EXPECT_FALSE(
      stateV1->getAssociatedSystemPortRangeIf(intf->getID()).has_value());
}

TEST(Interface, getInterfacePortsVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  EXPECT_EQ(getPortsForInterface(intf->getID(), stateV1).size(), 1);
}

TEST(Interface, getInterfacePorts) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  EXPECT_EQ(getPortsForInterface(intf->getID(), stateV1).size(), 11);
}

TEST(Interface, modify) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto multiIntfs = stateV1->getInterfaces();
  auto intf = multiIntfs->cbegin()->second->cbegin()->second;
  auto intfModified = intf->modify(&stateV1);
  EXPECT_EQ(intf.get(), intfModified);
  auto scope = multiIntfs->getNodeAndScope(intf->getID()).second;
  auto intfMap = multiIntfs->getMapNodeIf(scope);
  multiIntfs->publish();
  intfMap->publish();
  intf->publish();
  intfModified = intf->modify(&stateV1);
  EXPECT_NE(intf.get(), intfModified);
  EXPECT_NE(stateV1->getInterfaces()->getNode(intf->getID()), intf);
  EXPECT_NE(stateV1->getInterfaces()->getMapNodeIf(scope), intfMap);
  EXPECT_NE(stateV1->getInterfaces(), multiIntfs);
  auto oldMtu = intfModified->getMtu();
  auto newMtu = oldMtu + 1000;
  intfModified->setMtu(newMtu);
  EXPECT_EQ(stateV1->getInterfaces()->getNode(intf->getID())->getMtu(), newMtu);
}

TEST(Interface, getAllNodes) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_EQ(
      *stateV1->getInterfaces()->getAllNodes(),
      *utility::getFirstMap(stateV1->getInterfaces()));
}

TEST(Interface, getRemoteInterfaceType) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  config.dsfNodes()->insert({5, makeDsfNodeCfg(5)});
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);

  auto verifyRemoteInterfaceTypeHelper =
      [](const std::shared_ptr<MultiSwitchInterfaceMap>& intfs,
         const std::optional<RemoteInterfaceType>& expectedType) {
        EXPECT_GT(intfs->size(), 0);
        for (const auto& [_, intfMap] : std::as_const(*intfs)) {
          for (const auto& [_, intf] : std::as_const(*intfMap)) {
            EXPECT_EQ(intf->getRemoteInterfaceType(), expectedType);
          }
        }
      };

  // Local interfaces don't have remoteInterfaceType
  verifyRemoteInterfaceTypeHelper(
      stateV1->getInterfaces(),
      std::optional<RemoteInterfaceType>(std::nullopt));

  // Only statically programmed remote interfaces should be present given the
  // config applied.
  verifyRemoteInterfaceTypeHelper(
      stateV2->getRemoteInterfaces(),
      std::optional<RemoteInterfaceType>(RemoteInterfaceType::STATIC_ENTRY));
}
