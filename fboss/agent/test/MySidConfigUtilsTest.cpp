// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/rib/MySidConfigUtils.h"

using namespace facebook::fboss;

TEST(MySidConfigUtilsTest, BuildSidAddress) {
  auto locatorAddr = folly::IPAddressV6("3001:db8::");
  // Function ID "ffff" at offset 32 bits
  auto sid = buildSidAddress(locatorAddr, 32, "ffff");
  EXPECT_EQ(sid, folly::IPAddressV6("3001:db8:ffff::"));

  // Function ID "1"
  auto sid2 = buildSidAddress(locatorAddr, 32, "1");
  EXPECT_EQ(sid2, folly::IPAddressV6("3001:db8:1::"));

  // Function ID "7fff"
  auto sid3 = buildSidAddress(locatorAddr, 32, "7fff");
  EXPECT_EQ(sid3, folly::IPAddressV6("3001:db8:7fff::"));
}

TEST(MySidConfigUtilsTest, BuildSidAddressInvalidFunctionId) {
  auto locatorAddr = folly::IPAddressV6("3001:db8::");
  EXPECT_THROW(buildSidAddress(locatorAddr, 32, "zzz"), FbossError);
}

TEST(MySidConfigUtilsTest, BuildPortNameMapPhysicalPort) {
  cfg::SwitchConfig config;
  cfg::Port port;
  port.logicalID() = 1;
  port.name() = "eth2/5/1";
  port.ingressVlan() = 2001;
  config.ports()->push_back(port);

  auto portMap = buildPortNameToInterfaceIdMap(config);
  ASSERT_EQ(portMap.count("eth2/5/1"), 1);
  EXPECT_EQ(portMap.at("eth2/5/1"), InterfaceID(2001));
}

TEST(MySidConfigUtilsTest, BuildPortNameMapPhysicalPortDefaultName) {
  // When the port has no name set in cfg, fall back to "port-<logicalID>".
  cfg::SwitchConfig config;
  cfg::Port port;
  port.logicalID() = 7;
  port.ingressVlan() = 1234;
  config.ports()->push_back(port);

  auto portMap = buildPortNameToInterfaceIdMap(config);
  ASSERT_EQ(portMap.count("port-7"), 1);
  EXPECT_EQ(portMap.at("port-7"), InterfaceID(1234));
}

TEST(MySidConfigUtilsTest, BuildPortNameMapAggregatePort) {
  cfg::SwitchConfig config;
  cfg::AggregatePort aggPort;
  aggPort.key() = 301;
  aggPort.name() = "Port-Channel301";
  aggPort.description() = "test lag";
  config.aggregatePorts()->push_back(aggPort);

  auto portMap = buildPortNameToInterfaceIdMap(config);
  ASSERT_EQ(portMap.count("Port-Channel301"), 1);
  EXPECT_EQ(portMap.at("Port-Channel301"), InterfaceID(301));
}

TEST(MySidConfigUtilsTest, BuildPortNameMapEmpty) {
  cfg::SwitchConfig config;
  auto portMap = buildPortNameToInterfaceIdMap(config);
  EXPECT_TRUE(portMap.empty());
}

TEST(MySidConfigUtilsTest, ConvertDecapConfig) {
  cfg::MySidConfig config;
  config.locatorPrefix() = "3001:db8::/32";
  cfg::MySidEntryConfig entryConfig;
  cfg::DecapMySidConfig decapConfig;
  entryConfig.decap() = decapConfig;
  config.entries()[0x7fff] = entryConfig;

  auto result = convertMySidConfig(config, /*portNameToInterfaceId*/ {});
  ASSERT_EQ(result.mySids.size(), 1);
  EXPECT_EQ(result.mySids[0]->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_TRUE(result.unresolvedNextHops[0].empty());
  EXPECT_EQ(result.mySids[0]->getClientId(), ClientID::STATIC_ROUTE);
  EXPECT_FALSE(result.mySids[0]->getAdjacencyInterfaceId().has_value());

  // Verify SID address
  auto cidr = result.mySids[0]->getMySid();
  EXPECT_EQ(cidr.first, folly::IPAddress("3001:db8:7fff::"));
  EXPECT_EQ(cidr.second, 48);
}

TEST(MySidConfigUtilsTest, ConvertNodeConfig) {
  cfg::MySidConfig config;
  config.locatorPrefix() = "3001:db8::/32";
  cfg::MySidEntryConfig entryConfig;
  cfg::NodeMySidConfig nodeConfig;
  nodeConfig.nodeAddress() = "fc00:100::1";
  entryConfig.node() = nodeConfig;
  config.entries()[3] = entryConfig;

  auto result = convertMySidConfig(config, /*portNameToInterfaceId*/ {});
  ASSERT_EQ(result.mySids.size(), 1);
  EXPECT_EQ(result.mySids[0]->getType(), MySidType::NODE_MICRO_SID);
  EXPECT_EQ(result.mySids[0]->getClientId(), ClientID::STATIC_ROUTE);
  EXPECT_FALSE(result.mySids[0]->getAdjacencyInterfaceId().has_value());

  // Verify unresolved next hops contain the node address
  ASSERT_EQ(result.unresolvedNextHops[0].size(), 1);
  auto nhop = *result.unresolvedNextHops[0].begin();
  EXPECT_EQ(nhop.addr(), folly::IPAddress("fc00:100::1"));
}

TEST(MySidConfigUtilsTest, ConvertAdjacencyConfig) {
  std::unordered_map<std::string, InterfaceID> portNameToIntfId;
  portNameToIntfId["eth1/1/1"] = InterfaceID(2001);

  cfg::MySidConfig config;
  config.locatorPrefix() = "3001:db8::/32";
  cfg::MySidEntryConfig entryConfig;
  cfg::AdjacencyMySidConfig adjConfig;
  adjConfig.portName() = "eth1/1/1";
  entryConfig.adjacency() = adjConfig;
  config.entries()[1] = entryConfig;

  auto result = convertMySidConfig(config, portNameToIntfId);
  ASSERT_EQ(result.mySids.size(), 1);
  EXPECT_EQ(result.mySids[0]->getType(), MySidType::ADJACENCY_MICRO_SID);
  EXPECT_EQ(result.mySids[0]->getClientId(), ClientID::STATIC_ROUTE);
  // No unresolved next hops — resolved later via adjacencyInterfaceId
  EXPECT_TRUE(result.unresolvedNextHops[0].empty());
  // InterfaceID should be stored on the MySid object
  ASSERT_TRUE(result.mySids[0]->getAdjacencyInterfaceId().has_value());
  EXPECT_EQ(result.mySids[0]->getAdjacencyInterfaceId().value(), 2001);
}

TEST(MySidConfigUtilsTest, ConvertAdjacencyConfigUnknownPortThrows) {
  cfg::MySidConfig config;
  config.locatorPrefix() = "3001:db8::/32";
  cfg::MySidEntryConfig entryConfig;
  cfg::AdjacencyMySidConfig adjConfig;
  adjConfig.portName() = "missing-port";
  entryConfig.adjacency() = adjConfig;
  config.entries()[1] = entryConfig;

  EXPECT_THROW(
      convertMySidConfig(config, /*portNameToInterfaceId*/ {}), FbossError);
}

TEST(MySidConfigUtilsTest, ConvertMixedConfig) {
  std::unordered_map<std::string, InterfaceID> portNameToIntfId;
  portNameToIntfId["eth1/1/1"] = InterfaceID(2001);
  portNameToIntfId["Port-Channel301"] = InterfaceID(301);

  cfg::MySidConfig config;
  config.locatorPrefix() = "3001:db8::/32";

  // Decap entry
  cfg::MySidEntryConfig decapEntry;
  decapEntry.decap() = cfg::DecapMySidConfig{};
  config.entries()[0x7fff] = decapEntry;

  // Adjacency via physical port
  cfg::MySidEntryConfig adjEntry;
  cfg::AdjacencyMySidConfig adjConfig;
  adjConfig.portName() = "eth1/1/1";
  adjEntry.adjacency() = adjConfig;
  config.entries()[1] = adjEntry;

  // Adjacency via LAG
  cfg::MySidEntryConfig lagEntry;
  cfg::AdjacencyMySidConfig lagConfig;
  lagConfig.portName() = "Port-Channel301";
  lagEntry.adjacency() = lagConfig;
  config.entries()[2] = lagEntry;

  // Node entry
  cfg::MySidEntryConfig nodeEntry;
  cfg::NodeMySidConfig nodeConfig;
  nodeConfig.nodeAddress() = "fc00:200::1";
  nodeEntry.node() = nodeConfig;
  config.entries()[3] = nodeEntry;

  auto result = convertMySidConfig(config, portNameToIntfId);
  EXPECT_EQ(result.mySids.size(), 4);
  EXPECT_EQ(result.unresolvedNextHops.size(), 4);

  // Find entries by ID and verify properties
  std::unordered_map<std::string, std::shared_ptr<MySid>> byId;
  for (const auto& mySid : result.mySids) {
    byId[mySid->getID()] = mySid;
  }
  // Adjacency entries should have interface IDs
  ASSERT_NE(byId.find("3001:db8:1::/48"), byId.end());
  EXPECT_EQ(byId["3001:db8:1::/48"]->getAdjacencyInterfaceId().value(), 2001);
  ASSERT_NE(byId.find("3001:db8:2::/48"), byId.end());
  EXPECT_EQ(byId["3001:db8:2::/48"]->getAdjacencyInterfaceId().value(), 301);
  // All entries should have STATIC_ROUTE clientId
  for (const auto& mySid : result.mySids) {
    EXPECT_EQ(mySid->getClientId(), ClientID::STATIC_ROUTE);
  }
}
