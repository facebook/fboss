/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @file TunManagerRouteProcessorTest.cpp
 * @brief Unit tests for TunManager's routeProcessor functionality
 * @author Ashutosh Grewal (agrewal@meta.com)
 */

/**
 * @def TUNMANAGER_ROUTE_PROCESSOR_FRIEND_TESTS
 * @brief Macro defining friend declarations for TunManager route processor
 * tests
 *
 * This macro contains all the friend class and FRIEND_TEST declarations needed
 * to access private members of TunManager for testing the routeProcessor
 * functionality.
 */
#define TUNMANAGER_ROUTE_PROCESSOR_FRIEND_TESTS                                \
  friend class TunManagerRouteProcessorTest;                                   \
  friend class TunManagerAddressRuleTest;                                      \
  FRIEND_TEST(TunManagerRouteProcessorTest, ProcessIPv4DefaultRoute);          \
  FRIEND_TEST(TunManagerRouteProcessorTest, ProcessIPv6DefaultRoute);          \
  FRIEND_TEST(TunManagerRouteProcessorTest, SkipUnsupportedAddressFamily);     \
  FRIEND_TEST(TunManagerRouteProcessorTest, SkipInvalidTableId);               \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedRoutesIPv4Default);    \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedRoutesIPv6Default);    \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedRoutesBothV4AndV6);    \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedRoutesMultipleTables); \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedRoutesEmptyList);      \
  FRIEND_TEST(                                                                 \
      TunManagerRouteProcessorTest, DeleteProbedRoutesExceptionHandling);      \
  FRIEND_TEST(                                                                 \
      TunManagerAddressRuleTest,                                               \
      DeleteProbedAddressesAndRulesHandleExceptions);                          \
  FRIEND_TEST(                                                                 \
      TunManagerAddressRuleTest,                                               \
      DeleteProbedAddressesAndRulesAddressException);                          \
  FRIEND_TEST(                                                                 \
      TunManagerAddressRuleTest,                                               \
      DeleteProbedAddressesAndRulesSingleInterface);                           \
  FRIEND_TEST(                                                                 \
      TunManagerAddressRuleTest,                                               \
      DeleteProbedAddressesAndRulesMultipleInterfaces);                        \
  FRIEND_TEST(                                                                 \
      TunManagerAddressRuleTest,                                               \
      DeleteProbedAddressesAndRulesEmptyInterfaces);                           \
  FRIEND_TEST(TunManagerRouteProcessorTest, DeleteProbedInterfacesEmpty);

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <netlink/addr.h>
#include <netlink/object.h>
#include <netlink/route/nexthop.h>
#include <netlink/route/route.h>
}

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/test/MockTunIntf.h"
#include "fboss/agent/test/MockTunManager.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using ::testing::_;

namespace facebook::fboss {

/**
 * @class TunManagerRouteProcessorTest
 * @brief Test fixture for TunManager::routeProcessor functionality
 */
class TunManagerRouteProcessorTest : public ::testing::Test {
 public:
  /**
   * @brief Set up test environment before each test
   *
   * Creates mock platform, switch, and TUN manager instances.
   */
  void SetUp() override {
    platform_ = createMockPlatform();
    sw_ = setupMockSwitchWithoutHW(
        platform_.get(), nullptr, SwitchFlags::ENABLE_TUN);
    tunMgr_ = dynamic_cast<MockTunManager*>(sw_->getTunManager());
    ASSERT_NE(nullptr, tunMgr_);
  }

  /**
   * @brief Clean up test environment after each test
   *
   * Properly destroys mock objects in correct order to avoid memory
   * leaks.
   */
  void TearDown() override {
    // Reset in proper order: sw_ first, then platform_
    sw_.reset();
    platform_.reset();
  }

  static void addProbedRoute(
      TunManager* tunMgr,
      int family,
      int tableId,
      const std::string& destination,
      int ifIndex) {
    TunManager::ProbedRoute route;
    route.family = family;
    route.tableId = tableId;
    route.destination = destination;
    route.ifIndex = ifIndex;
    tunMgr->probedRoutes_.push_back(route);
  }

  /**
   * @brief Build interface index to table ID mapping from probed routes
   *
   * @param tunMgr Pointer to TunManager instance containing probed routes
   * @return std::unordered_map<int, int> Map from interface index to
   * table ID
   */
  static std::unordered_map<int, int> buildIfIndexToTableIdMap(
      TunManager* tunMgr) {
    std::unordered_map<int, int> ifIndexToTableId;
    for (const auto& probedRoute : tunMgr->probedRoutes_) {
      if (probedRoute.ifIndex > 0) {
        ifIndexToTableId[probedRoute.ifIndex] = probedRoute.tableId;
      }
    }
    return ifIndexToTableId;
  }

 protected:
  std::unique_ptr<MockPlatform> platform_; ///< Mock platform instance
  std::unique_ptr<SwSwitch> sw_; ///< Mock switch instance
  MockTunManager* tunMgr_{}; ///< Mock TUN manager for testing

  /**
   * @brief Helper function to test default route processing for both IPv4
   * and IPv6
   *
   * @param family Address family (AF_INET or AF_INET6)
   * @param tableId Routing table ID
   * @param ifIndex Interface index for nexthop
   * @param expectedDestination Expected destination string representation
   */
  void testProcessDefaultRoute(
      int family,
      int tableId,
      int ifIndex,
      const std::string& expectedDestination) {
    // Create a real netlink route object for default route
    auto route = rtnl_route_alloc();
    ASSERT_NE(nullptr, route);

    // Set up default route with specified parameters
    rtnl_route_set_family(route, family);
    rtnl_route_set_table(route, tableId);
    rtnl_route_set_protocol(route, TunManager::RTPROT_FBOSS);

    // Create destination address (default route)
    auto dst = nl_addr_build(family, nullptr, 0);
    ASSERT_NE(nullptr, dst);
    nl_addr_set_prefixlen(dst, 0);
    rtnl_route_set_dst(route, dst);

    // Add nexthop with interface index
    auto nexthop = rtnl_route_nh_alloc();
    ASSERT_NE(nullptr, nexthop);
    rtnl_route_nh_set_ifindex(nexthop, ifIndex);
    rtnl_route_add_nexthop(route, nexthop);

    // Call the actual routeProcessor function
    TunManager::routeProcessor(
        reinterpret_cast<struct nl_object*>(route),
        static_cast<void*>(tunMgr_));

    // Verify the route was stored by routeProcessor
    ASSERT_EQ(1, tunMgr_->probedRoutes_.size());

    const auto& probedRoute = tunMgr_->probedRoutes_[0];
    EXPECT_EQ(family, probedRoute.family);
    EXPECT_EQ(tableId, probedRoute.tableId);
    EXPECT_EQ(expectedDestination, probedRoute.destination);
    EXPECT_EQ(ifIndex, probedRoute.ifIndex);

    // Cleanup
    nl_addr_put(dst);
    rtnl_route_put(route);
  }
};

/**
 * @class TunManagerAddressRuleTest
 * @brief Test fixture for TunManager address rule functionality=
 */
class TunManagerAddressRuleTest : public TunManagerRouteProcessorTest {
 public:
  /**
   * @brief Helper method to add a mock interface with specified addresses
   *
   * @param ifID Interface ID
   * @param ifIndex Interface index
   * @param ifName Interface name
   * @param addresses Vector of IP address and prefix length pairs
   */
  void addMockInterface(
      const InterfaceID& ifID,
      int ifIndex,
      const std::string& ifName,
      const std::vector<std::pair<folly::IPAddress, uint8_t>>& addresses) {
    auto mockIntf = std::make_unique<MockTunIntf>(
        ifID,
        ifName,
        ifIndex,
        1500 // MTU
    );

    // Set the interface addresses using the test helper method
    Interface::Addresses addrs;
    for (const auto& addr : addresses) {
      addrs.emplace(addr.first, addr.second);
    }
    mockIntf->setTestAddresses(addrs);

    tunMgr_->intfs_[ifID] = std::move(mockIntf);
  }

  /**
   * @brief Helper method to create ifIndexToTableId mapping
   *
   * @param mappings Vector of (ifIndex, tableId) pairs
   * @return std::unordered_map<int, int> Map from interface index to
   * table ID
   */
  std::unordered_map<int, int> createIfIndexToTableIdMap(
      const std::vector<std::pair<int, int>>& mappings) {
    std::unordered_map<int, int> ifIndexToTableId;
    for (const auto& mapping : mappings) {
      ifIndexToTableId[mapping.first] = mapping.second;
    }
    return ifIndexToTableId;
  }
};

/**
 * @brief Test processing of IPv4 default route
 *
 * Verifies that routeProcessor correctly processes and stores an IPv4
 * default route (0.0.0.0/0) with valid parameters.
 */
TEST_F(TunManagerRouteProcessorTest, ProcessIPv4DefaultRoute) {
  testProcessDefaultRoute(AF_INET, 100, 42, "0.0.0.0/0");
}

/**
 * @brief Test processing of IPv6 default route
 *
 * Verifies that routeProcessor correctly processes and stores an IPv6
 * default route (::/0) with valid parameters.
 */
TEST_F(TunManagerRouteProcessorTest, ProcessIPv6DefaultRoute) {
  testProcessDefaultRoute(AF_INET6, 150, 24, "::/0");
}

/**
 * @brief Test filtering of unsupported address families
 *
 * Verifies that routeProcessor correctly filters out and ignores routes
 * with unsupported address families. The function should only process
 * AF_INET and AF_INET6 routes, ignoring all others.
 */
TEST_F(TunManagerRouteProcessorTest, SkipUnsupportedAddressFamily) {
  // Create a route with unsupported address family
  auto route = rtnl_route_alloc();
  ASSERT_NE(nullptr, route);

  rtnl_route_set_family(route, AF_UNIX); // Unsupported family
  rtnl_route_set_table(route, 100);

  // Call the routeProcessor function
  TunManager::routeProcessor(
      reinterpret_cast<struct nl_object*>(route), static_cast<void*>(tunMgr_));

  // Verify no routes were stored
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());

  // Cleanup
  rtnl_route_put(route);
}

/**
 * @brief Test filtering of invalid table IDs
 *
 * Verifies that routeProcessor correctly filters out routes with invalid
 * table IDs. The function should only process routes with table IDs in
 * the range [1-253], as table IDs 0, 254, and 255 are reserved by the
 * kernel.
 *
 * This test uses table ID 254 (outside the valid range) and expects that
 * no routes are stored in probedRoutes_, demonstrating proper table ID
 * validation.
 */
TEST_F(TunManagerRouteProcessorTest, SkipInvalidTableId) {
  // Create a route with invalid table ID
  auto route = rtnl_route_alloc();
  ASSERT_NE(nullptr, route);

  rtnl_route_set_family(route, AF_INET);
  rtnl_route_set_table(route, 254); // Outside valid range [1-253]

  // Call the routeProcessor function
  TunManager::routeProcessor(
      reinterpret_cast<struct nl_object*>(route), static_cast<void*>(tunMgr_));

  // Verify no routes were stored
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());

  // Cleanup
  rtnl_route_put(route);
}

/**
 * @brief Test deletion of probed IPv4 default routes
 *
 * Verifies that deleteProbedRoutes correctly identifies and removes IPv4
 * default routes (0.0.0.0/0) by calling addRemoveRouteTable with delete
 * flag.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesIPv4Default) {
  // Add IPv4 default route to probed routes
  addProbedRoute(tunMgr_, AF_INET, 100, "0.0.0.0/0", 42);

  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(100, 42, false, ::testing::Eq(std::nullopt)))
      .Times(1);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes
  tunMgr_->deleteProbedRoutes(ifIndexToTableId);

  // Verify probed routes are cleared;
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());
}

/**
 * @brief Test deletion of probed IPv6 default routes
 *
 * Verifies that deleteProbedRoutes correctly identifies and removes IPv6
 * default routes (::/0) by calling addRemoveRouteTable with delete flag.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesIPv6Default) {
  // Add IPv6 default route to probed routes
  addProbedRoute(tunMgr_, AF_INET6, 150, "::/0", 24);

  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(150, 24, false, ::testing::Eq(std::nullopt)))
      .Times(1);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes
  tunMgr_->deleteProbedRoutes(ifIndexToTableId);

  // Verify probed routes are cleared;
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());
}

/**
 * @brief Test deletion of probed routes with both IPv4 and IPv6 default
 * routes
 *
 * Verifies that deleteProbedRoutes correctly handles multiple routes
 * (both IPv4 and IPv6) but only makes one call to addRemoveRouteTable.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesBothV4AndV6) {
  // Add both IPv4 and IPv6 default routes to probed routes
  addProbedRoute(tunMgr_, AF_INET, 100, "0.0.0.0/0", 42);
  addProbedRoute(tunMgr_, AF_INET6, 100, "::/0", 42);

  // Expect only one call to addRemoveRouteTable despite having two routes
  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(100, 42, false, ::testing::Eq(std::nullopt)))
      .Times(1);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes
  tunMgr_->deleteProbedRoutes(ifIndexToTableId);

  // Verify probed routes are cleared
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());
}

/**
 * @brief Test deletion of probed routes with IPv4 and IPv6 default routes
 * in two tables
 *
 * Verifies that deleteProbedRoutes correctly handles in  different table
 * IDs and makes separate calls to addRemoveRouteTable for each table.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesMultipleTables) {
  // Add IPv4 default route to table 100 and IPv6 default route to table
  // 200
  addProbedRoute(tunMgr_, AF_INET, 100, "0.0.0.0/0", 42);
  addProbedRoute(tunMgr_, AF_INET6, 100, "::/0", 42);

  addProbedRoute(tunMgr_, AF_INET, 200, "0.0.0.0/0", 24);
  addProbedRoute(tunMgr_, AF_INET6, 200, "::/0", 24);

  // Expect two separate calls to addRemoveRouteTable for different tables
  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(100, 42, false, ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(200, 24, false, ::testing::Eq(std::nullopt)))
      .Times(1);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes
  tunMgr_->deleteProbedRoutes(ifIndexToTableId);

  // Verify probed routes are cleared
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());
}

/**
 * @brief Test deletion of probed routes when no routes exist
 *
 * Verifies that deleteProbedRoutes correctly handles the case when there
 * are no routes in probedRoutes_. Should not make any calls to
 * addRemoveRouteTable.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesEmptyList) {
  // Verify probedRoutes_ starts empty
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());

  // Expect no calls to addRemoveRouteTable since there are no routes
  EXPECT_CALL(*tunMgr_, addRemoveRouteTable(_, _, _, _)).Times(0);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes
  tunMgr_->deleteProbedRoutes(ifIndexToTableId);

  // Verify probedRoutes_ remains empty
  EXPECT_EQ(0, tunMgr_->probedRoutes_.size());
}

/**
 * @brief Test exception handling in deleteProbedRoutes
 *
 * Verifies that deleteProbedRoutes propagates exceptions thrown by
 * addRemoveRouteTable. When an exception occurs, the method should throw
 * and stop processing, leaving the probedRoutes_ list unchanged.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedRoutesExceptionHandling) {
  // Add multiple default routes in different tables
  addProbedRoute(tunMgr_, AF_INET, 100, "0.0.0.0/0", 42);
  addProbedRoute(tunMgr_, AF_INET6, 100, "::/0", 42);
  addProbedRoute(tunMgr_, AF_INET, 200, "0.0.0.0/0", 24);
  addProbedRoute(tunMgr_, AF_INET6, 200, "::/0", 24);

  // Set up mock to throw exception for table 100 but succeed for table
  // 200
  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(100, 42, false, ::testing::Eq(std::nullopt)))
      .Times(1)
      .WillOnce(::testing::Throw(std::runtime_error("Netlink error")));

  EXPECT_CALL(
      *tunMgr_,
      addRemoveRouteTable(200, 24, false, ::testing::Eq(std::nullopt)))
      .Times(1);

  // Build ifIndexToTableId map from probed routes
  auto ifIndexToTableId = buildIfIndexToTableIdMap(tunMgr_);

  // Call deleteProbedRoutes - should throw when addRemoveRouteTable
  // throws
  EXPECT_THROW(
      tunMgr_->deleteProbedRoutes(ifIndexToTableId), std::runtime_error);

  // Verify probed routes are not cleared when exceptions occur
  EXPECT_EQ(tunMgr_->probedRoutes_.size(), 4);
}

/**
 * @brief Test deletion of source IP rules and TUN addresses for a single
 * interface
 *
 * Verifies cleanup of source IP routing rules and TUN interface addresses
 * for a single interface with mixed IPv4/IPv6 addresses.
 */
TEST_F(
    TunManagerAddressRuleTest,
    DeleteProbedAddressesAndRulesSingleInterface) {
  // Add a mock interface with addresses
  std::vector<std::pair<folly::IPAddress, uint8_t>> addresses = {
      {folly::IPAddress("10.1.1.1"), 24},
      {folly::IPAddress("2001:db8::1"), 64}};

  addMockInterface(InterfaceID(2000), 42, "fboss2000", addresses);

  // Create ifIndexToTableId map
  auto ifIndexToTableId = createIfIndexToTableIdMap({{42, 100}});

  // Expect calls to delete source rules and addresses
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("10.1.1.1"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("2001:db8::1"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("10.1.1.1"), 24, false))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("2001:db8::1"), 64, false))
      .Times(1);

  // Call deleteProbedAddressesAndRules
  tunMgr_->deleteProbedAddressesAndRules(ifIndexToTableId);
}

/**
 * @brief Test deletion of source IP rules and TUN addresses across
 * multiple interfaces
 *
 * Verifies cleanup of source IP routing rules (source IP → routing table
 * mapping) and TUN interface addresses for multiple interfaces with
 * different routing tables.
 */
TEST_F(
    TunManagerAddressRuleTest,
    DeleteProbedAddressesAndRulesMultipleInterfaces) {
  // Add multiple mock interfaces with addresses
  std::vector<std::pair<folly::IPAddress, uint8_t>> addresses1 = {
      {folly::IPAddress("10.1.1.1"), 24}};
  std::vector<std::pair<folly::IPAddress, uint8_t>> addresses2 = {
      {folly::IPAddress("10.2.2.2"), 24},
      {folly::IPAddress("2001:db8::2"), 64}};

  addMockInterface(InterfaceID(2000), 42, "fboss2000", addresses1);
  addMockInterface(InterfaceID(2001), 43, "fboss2001", addresses2);

  // Create ifIndexToTableId map
  auto ifIndexToTableId = createIfIndexToTableIdMap({{42, 100}, {43, 101}});

  // Expect calls to delete source rules and addresses for both interfaces
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("10.1.1.1"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          101,
          folly::IPAddress("10.2.2.2"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          101,
          folly::IPAddress("2001:db8::2"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("10.1.1.1"), 24, false))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2001", 43, folly::IPAddress("10.2.2.2"), 24, false))
      .Times(1);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2001", 43, folly::IPAddress("2001:db8::2"), 64, false))
      .Times(1);

  // Call deleteProbedAddressesAndRules
  tunMgr_->deleteProbedAddressesAndRules(ifIndexToTableId);
}

/**
 * Test that deleteProbedAddressesAndRules propagates exceptions and stops
 * processing when addRemoveSourceRouteRule throws.
 */
TEST_F(
    TunManagerAddressRuleTest,
    DeleteProbedAddressesAndRulesHandleExceptions) {
  // Add a mock interface with addresses
  std::vector<std::pair<folly::IPAddress, uint8_t>> addresses = {
      {folly::IPAddress("10.1.1.1"), 24}, {folly::IPAddress("10.1.1.2"), 24}};

  addMockInterface(InterfaceID(2000), 42, "fboss2000", addresses);

  // Create ifIndexToTableId map
  auto ifIndexToTableId = createIfIndexToTableIdMap({{42, 100}});

  // Make the first addRemoveSourceRouteRule call throw an exception
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("10.1.1.1"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1)
      .WillOnce(
          ::testing::Throw(std::runtime_error("Mock source rule exception")));

  // Since no exception handling exists, the function should throw and not
  // continue processing remaining addresses
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("10.1.1.2"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(0);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("10.1.1.1"), 24, false))
      .Times(0);
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("10.1.1.2"), 24, false))
      .Times(0);

  // Call deleteProbedAddressesAndRules - should throw exception since no
  // exception handling
  EXPECT_THROW(
      tunMgr_->deleteProbedAddressesAndRules(ifIndexToTableId),
      std::runtime_error);
}

/**
 * Test that deleteProbedAddressesAndRules handles exceptions gracefully
 * when addRemoveTunAddress throws during address deletion.
 */
TEST_F(
    TunManagerAddressRuleTest,
    DeleteProbedAddressesAndRulesAddressException) {
  // Add a mock interface with addresses
  std::vector<std::pair<folly::IPAddress, uint8_t>> addresses = {
      {folly::IPAddress("10.1.1.1"), 24}};

  addMockInterface(InterfaceID(2000), 42, "fboss2000", addresses);

  // Create ifIndexToTableId map
  auto ifIndexToTableId = createIfIndexToTableIdMap({{42, 100}});

  // Source rule call should succeed
  EXPECT_CALL(
      *tunMgr_,
      addRemoveSourceRouteRule(
          100,
          folly::IPAddress("10.1.1.1"),
          false,
          ::testing::Eq(std::nullopt)))
      .Times(1);

  // Make addRemoveTunAddress throw an exception
  EXPECT_CALL(
      *tunMgr_,
      addRemoveTunAddress(
          "fboss2000", 42, folly::IPAddress("10.1.1.1"), 24, false))
      .Times(1)
      .WillOnce(::testing::Throw(std::runtime_error("Mock address exception")));

  // Call deleteProbedAddressesAndRules - should throw exception since no
  // exception handling for address deletion
  EXPECT_THROW(
      tunMgr_->deleteProbedAddressesAndRules(ifIndexToTableId),
      std::runtime_error);
}

/**
 * @brief Test deletion of source IP rules and TUN addresses with empty
 * interfaces
 *
 * Verifies that deleteProbedAddressesAndRules handles empty interface
 * mapping correctly without making any calls to addRemoveSourceRouteRule
 * or addRemoveTunAddress.
 */
TEST_F(
    TunManagerAddressRuleTest,
    DeleteProbedAddressesAndRulesEmptyInterfaces) {
  // Clear any existing interfaces (handled by test setup/teardown)

  // Create empty ifIndexToTableId map
  std::unordered_map<int, int> ifIndexToTableId;

  // Expect no calls to addRemoveSourceRouteRule or addRemoveTunAddress
  EXPECT_CALL(*tunMgr_, addRemoveSourceRouteRule(_, _, _, _)).Times(0);
  EXPECT_CALL(*tunMgr_, addRemoveTunAddress(_, _, _, _, _)).Times(0);

  // Call deleteProbedAddressesAndRules with empty interfaces
  tunMgr_->deleteProbedAddressesAndRules(ifIndexToTableId);
}

/**
 * @brief Test deleteProbedInterfaces with empty interfaces map
 *
 * Verifies that deleteProbedInterfaces handles empty interfaces map
 * correctly.
 */
TEST_F(TunManagerRouteProcessorTest, DeleteProbedInterfacesEmpty) {
  // Verify interfaces map starts empty
  EXPECT_EQ(0, tunMgr_->intfs_.size());

  // Call deleteProbedInterfaces with empty map
  tunMgr_->deleteProbedInterfaces();

  // Verify interfaces map remains empty
  EXPECT_EQ(0, tunMgr_->intfs_.size());
}

} // namespace facebook::fboss
