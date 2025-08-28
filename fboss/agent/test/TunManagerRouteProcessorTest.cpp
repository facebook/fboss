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
#define TUNMANAGER_ROUTE_PROCESSOR_FRIEND_TESTS \
  friend class TunManagerRouteProcessorTest;    \
  FRIEND_TEST(TunManagerRouteProcessorTest, ProcessIPv4DefaultRoute);

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
   * Properly destroys mock objects in correct order to avoid memory leaks.
   */
  void TearDown() override {
    // Reset in proper order: sw_ first, then platform_
    sw_.reset();
    platform_.reset();
  }

 protected:
  std::unique_ptr<MockPlatform> platform_; ///< Mock platform instance
  std::unique_ptr<SwSwitch> sw_; ///< Mock switch instance
  MockTunManager* tunMgr_{}; ///< Mock TUN manager for testing
};

/**
 * @brief Test processing of IPv4 default route
 *
 * Verifies that routeProcessor correctly processes and stores an IPv4 default
 * route (0.0.0.0/0) with valid parameters. The test creates a real netlink
 * route object with:
 * - IPv4 address family (AF_INET)
 * - Valid table ID (100, within range [1-253])
 * - Default destination (0.0.0.0/0)
 * - Mock interface index (42)
 *
 * Expects the route to be stored in probedRoutes_ with all fields correctly
 * extracted and parsed.
 */
TEST_F(TunManagerRouteProcessorTest, ProcessIPv4DefaultRoute) {
  // Create a real netlink route object for IPv4 default route
  auto route = rtnl_route_alloc();
  ASSERT_NE(nullptr, route);

  // Set up IPv4 default route (0.0.0.0/0)
  rtnl_route_set_family(route, AF_INET);
  rtnl_route_set_table(route, 100); // Table ID within valid range [1-253]
  rtnl_route_set_protocol(route, RTPROT_STATIC);

  // Create destination address (0.0.0.0/0 for default route)
  auto dst = nl_addr_build(AF_INET, nullptr, 0);
  ASSERT_NE(nullptr, dst);
  nl_addr_set_prefixlen(dst, 0);
  rtnl_route_set_dst(route, dst);

  // Add nexthop with interface index
  auto nexthop = rtnl_route_nh_alloc();
  ASSERT_NE(nullptr, nexthop);
  rtnl_route_nh_set_ifindex(nexthop, 42); // Mock interface index
  rtnl_route_add_nexthop(route, nexthop);

  // Call the actual routeProcessor function
  TunManager::routeProcessor(
      reinterpret_cast<struct nl_object*>(route), static_cast<void*>(tunMgr_));

  // Verify the route was stored by routeProcessor
  ASSERT_EQ(1, tunMgr_->probedRoutes_.size());

  const auto& probedRoute = tunMgr_->probedRoutes_[0];
  EXPECT_EQ(AF_INET, probedRoute.family);
  EXPECT_EQ(100, probedRoute.tableId);
  EXPECT_EQ("0.0.0.0/0", probedRoute.destination);
  EXPECT_EQ(42, probedRoute.ifIndex);

  // Cleanup
  nl_addr_put(dst);
  rtnl_route_put(route);
}
} // namespace facebook::fboss
