/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gtest/gtest.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

namespace {

constexpr auto DESIRED_PEER_ADDRESS_IPV4 = "10.0.5.2/24";
constexpr auto DESIRED_PEER_ADDRESS_IPV4_VLAN = "10.0.0.2/24";
constexpr auto VLAN_ID = 100;
constexpr auto INTERFACE_ID = 100;
constexpr auto PORT_ID = 1;
constexpr auto PORT_ID_5 = 5;

/**
 * Create a VLAN interface config with desiredPeerAddressIPv4
 */
cfg::SwitchConfig createVlanInterfaceConfigForArp() {
  cfg::SwitchConfig config;
  config.switchSettings()->switchIdToSwitchInfo() = {
      std::make_pair(0, createSwitchInfo(cfg::SwitchType::NPU))};

  config.vlans()->resize(1);
  *config.vlans()[0].name() = "TestVlan";
  *config.vlans()[0].id() = VLAN_ID;
  *config.vlans()[0].routable() = true;
  config.vlans()[0].intfID() = INTERFACE_ID;

  config.vlanPorts()->resize(1);
  config.ports()->resize(1);

  preparedMockPortConfig(config.ports()[0], PORT_ID);
  *config.ports()[0].routable() = true;
  *config.ports()[0].ingressVlan() = VLAN_ID;

  *config.vlanPorts()[0].vlanID() = VLAN_ID;
  *config.vlanPorts()[0].logicalPort() = PORT_ID;
  *config.vlanPorts()[0].spanningTreeState() =
      cfg::SpanningTreeState::FORWARDING;
  *config.vlanPorts()[0].emitTags() = 0;

  config.interfaces()->resize(1);
  *config.interfaces()[0].intfID() = INTERFACE_ID;
  *config.interfaces()[0].vlanID() = VLAN_ID;
  *config.interfaces()[0].routerID() = 0;
  config.interfaces()[0].type() = cfg::InterfaceType::VLAN;
  config.interfaces()[0].name() = "VlanInterface100";
  config.interfaces()[0].mtu() = 1500;
  config.interfaces()[0].ipAddresses()->resize(1);
  config.interfaces()[0].ipAddresses()[0] = "10.0.0.1/24";

  config.interfaces()[0].desiredPeerAddressIPv4() =
      DESIRED_PEER_ADDRESS_IPV4_VLAN;

  return config;
}

} // namespace

/**
 * Test fixture for ARP cold start tests
 */
class PortUpdateHandlerARPTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_disable_neighbor_updates = false;
    FLAGS_arp_static_neighbor = true;
  }

  std::unique_ptr<HwTestHandle> setupTestHandle(cfg::SwitchConfig config) {
    auto handle = createTestHandle(&config);
    sw_ = handle->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
    return handle;
  }

 protected:
  /**
   * Verify ARP request was sent for a given interface and target IPv4
   */
  void verifyArpRequest(
      SwSwitch* sw,
      InterfaceID intfId,
      const std::string& expectedDesiredPeer,
      cfg::InterfaceType expectedType) {
    auto state = sw->getState();
    auto interfaces = state->getInterfaces();
    auto intf = interfaces->getNode(intfId);
    EXPECT_NE(intf, nullptr);

    // Verify interface type
    EXPECT_EQ(intf->getType(), expectedType);

    // Check if desiredPeerAddressIPv4 is configured
    auto desiredPeer = intf->getDesiredPeerAddressIPv4();
    EXPECT_TRUE(desiredPeer.has_value());
    EXPECT_EQ(desiredPeer.value(), expectedDesiredPeer);

    // Parse the target IP
    auto cidrNetwork = folly::IPAddress::createNetwork(*desiredPeer, -1, false);
    auto targetIP = cidrNetwork.first.asV4();

    // Verify that ARP request was sent by checking ARP cache
    bool foundEntry = false;
    XLOG(DBG2) << "verifyArpRequest: checking ARP cache for " << targetIP.str();
    WITH_RETRIES({
      auto arpCache = sw->getNeighborUpdater()->getArpCacheDataForIntf().get();
      XLOG(DBG2) << "ARP cache size: " << arpCache.size();
      auto cacheEntry = std::find_if(
          arpCache.begin(), arpCache.end(), [&targetIP](const auto& entry) {
            return entry.ip() == facebook::network::toBinaryAddress(targetIP);
          });
      EXPECT_EVENTUALLY_TRUE(cacheEntry != arpCache.end());
      foundEntry = (cacheEntry != arpCache.end());
    });
    EXPECT_TRUE(foundEntry);
    if (foundEntry) {
      XLOG(INFO) << "verifyArpRequest: CONFIRMED - ARP request was sent for "
                 << targetIP.str();
    }
  }

  SwSwitch* sw_{};
};

/**
 * Test that port up triggers ARP request for VLAN interface
 */
TEST_F(PortUpdateHandlerARPTest, VlanInterfacePortUp) {
  auto config = createVlanInterfaceConfigForArp();
  auto handle = setupTestHandle(config);

  // Initially set port to DOWN
  sw_->linkStateChanged(PortID(PORT_ID), false, cfg::PortType::INTERFACE_PORT);
  {
    auto arpCache = sw_->getNeighborUpdater()->getArpCacheDataForIntf().get();
    EXPECT_EQ(arpCache.size(), 0);
  }

  // Bring port UP - should trigger ARP request
  sw_->linkStateChanged(PortID(PORT_ID), true, cfg::PortType::INTERFACE_PORT);

  verifyArpRequest(
      sw_,
      InterfaceID(INTERFACE_ID),
      DESIRED_PEER_ADDRESS_IPV4_VLAN,
      cfg::InterfaceType::VLAN);
}

/**
 * Test that no ARP request is sent when desiredPeerAddressIPv4 is not
 * configured
 */
TEST_F(PortUpdateHandlerARPTest, VlanInterfaceNoDesiredPeer) {
  auto config = createVlanInterfaceConfigForArp();
  // Remove desiredPeerAddressIPv4
  config.interfaces()[0].desiredPeerAddressIPv4().reset();

  auto handle = setupTestHandle(config);

  sw_->linkStateChanged(PortID(PORT_ID), false, cfg::PortType::INTERFACE_PORT);
  sw_->linkStateChanged(PortID(PORT_ID), true, cfg::PortType::INTERFACE_PORT);

  // Verify no ARP entry was created
  auto state = sw_->getState();
  auto intf = state->getInterfaces()->getNode(InterfaceID(INTERFACE_ID));
  EXPECT_NE(intf, nullptr);
  EXPECT_FALSE(intf->getDesiredPeerAddressIPv4().has_value());

  auto arpCache = sw_->getNeighborUpdater()->getArpCacheDataForIntf().get();
  // No proactive ARP entry expected
  folly::IPAddressV4 targetIP("10.0.0.2");
  auto cacheEntry = std::find_if(
      arpCache.begin(), arpCache.end(), [&targetIP](const auto& entry) {
        return entry.ip() == facebook::network::toBinaryAddress(targetIP);
      });
  EXPECT_TRUE(cacheEntry == arpCache.end());
}

/**
 * Test that port up triggers ARP request for SYSTEM_PORT interface (VOQ
 * switch)
 */
TEST_F(PortUpdateHandlerARPTest, SystemPortInterfacePortUp) {
  auto config = testConfigA(cfg::SwitchType::VOQ);
  // Add desiredPeerAddressIPv4 to the first interface
  // VOQ config has ipAddresses like "10.0.5.1/24" for port 5
  config.interfaces()[0].desiredPeerAddressIPv4() = DESIRED_PEER_ADDRESS_IPV4;

  auto handle = setupTestHandle(config);

  // Initially set port to DOWN
  sw_->linkStateChanged(
      PortID(PORT_ID_5), false, cfg::PortType::INTERFACE_PORT);

  // Bring port UP
  sw_->linkStateChanged(PortID(PORT_ID_5), true, cfg::PortType::INTERFACE_PORT);

  // Get the interface ID for the first VOQ interface
  auto state = sw_->getState();
  auto interfaces = state->getInterfaces();
  // Find the interface that has desiredPeerAddressIPv4
  InterfaceID targetIntfId(0);
  for (const auto& [_, intfMap] : std::as_const(*interfaces)) {
    for (const auto& [intfId, intf] : std::as_const(*intfMap)) {
      if (intf->getDesiredPeerAddressIPv4().has_value()) {
        targetIntfId = intfId;
        break;
      }
    }
  }
  ASSERT_NE(targetIntfId, InterfaceID(0));

  verifyArpRequest(
      sw_,
      targetIntfId,
      DESIRED_PEER_ADDRESS_IPV4,
      cfg::InterfaceType::SYSTEM_PORT);
}

/**
 * Test that ARP request is NOT sent when flag is disabled
 */
TEST_F(PortUpdateHandlerARPTest, FlagDisabledNoArp) {
  FLAGS_arp_static_neighbor = false;

  auto config = createVlanInterfaceConfigForArp();
  auto handle = setupTestHandle(config);

  sw_->linkStateChanged(PortID(PORT_ID), false, cfg::PortType::INTERFACE_PORT);
  sw_->linkStateChanged(PortID(PORT_ID), true, cfg::PortType::INTERFACE_PORT);

  // No ARP entry should be created when flag is off
  auto arpCache = sw_->getNeighborUpdater()->getArpCacheDataForIntf().get();
  folly::IPAddressV4 targetIP("10.0.0.2");
  auto cacheEntry = std::find_if(
      arpCache.begin(), arpCache.end(), [&targetIP](const auto& entry) {
        return entry.ip() == facebook::network::toBinaryAddress(targetIP);
      });
  EXPECT_TRUE(cacheEntry == arpCache.end());
}
