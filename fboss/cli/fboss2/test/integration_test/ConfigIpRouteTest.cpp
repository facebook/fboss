// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config protocol static ip/ipv6 route' and
 * 'delete protocol static ip/ipv6 route'
 *
 * This test:
 * 1. Adds an IP address to an interface to create a connected route
 *    (using 'config interface <name> ip-address/ipv6-address <prefix>')
 * 2. Adds a static route with a nexthop from that connected subnet
 * 3. Verifies the route was added via 'show route'
 * 4. Deletes the route (using 'delete protocol static ip/ipv6 route <prefix>')
 * 5. Verifies the route was removed
 * 6. Cleans up by deleting the interface IP address
 *    (using 'delete interface <interface> ip-address/ipv6-address <prefix>')
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigIpRouteTest : public Fboss2IntegrationTest {
 protected:
  /**
   * Add an IP address to an interface.
   * Command syntax: config interface <interface> ip-address <ip_prefix>
   *             or: config interface <interface> ipv6-address <ip_prefix>
   */
  void addInterfaceIp(
      const std::string& interface,
      const std::string& ipPrefix,
      bool isIpv6 = false) {
    std::string attr = isIpv6 ? "ipv6-address" : "ip-address";
    std::vector<std::string> args = {
        "config", "interface", interface, attr, ipPrefix};
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to add interface IP: " << result.stderr;
    commitConfig();
  }

  /**
   * Delete an IP address from an interface.
   * Command syntax: delete interface <interface> ip-address <ip_prefix>
   *             or: delete interface <interface> ipv6-address <ip_prefix>
   */
  void deleteInterfaceIp(
      const std::string& interface,
      const std::string& ipPrefix,
      bool isIpv6 = false) {
    std::string attr = isIpv6 ? "ipv6-address" : "ip-address";
    std::vector<std::string> args = {
        "delete", "interface", interface, attr, ipPrefix};
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete interface IP: " << result.stderr;
    commitConfig();
  }

  /**
   * Add a static route with a nexthop.
   */
  void addRoute(
      const std::string& prefix,
      const std::string& nexthop,
      bool isIpv6 = false) {
    std::vector<std::string> args;
    if (isIpv6) {
      args = {"config", "protocol", "static", "ipv6", "route", prefix, nexthop};
    } else {
      args = {"config", "protocol", "static", "ip", "route", prefix, nexthop};
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0) << "Failed to add route: " << result.stderr;
    commitConfig();
  }

  /**
   * Delete a static route.
   * Command syntax: delete protocol static ip route <prefix>
   *             or: delete protocol static ipv6 route <prefix>
   */
  void deleteRoute(const std::string& prefix, bool isIpv6 = false) {
    std::vector<std::string> args;
    if (isIpv6) {
      args = {"delete", "protocol", "static", "ipv6", "route", prefix};
    } else {
      args = {"delete", "protocol", "static", "ip", "route", prefix};
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete route: " << result.stderr;
    commitConfig();
  }

  /**
   * Check if a route exists in the routing table.
   * Optionally verify the nexthops or that it's a drop route.
   *
   * @param prefix The network prefix to look for
   * @param expectedNexthops If provided, verify these nexthops exist
   * @param expectDrop If true, verify the route has no nexthops (drop route)
   */
  bool routeExists(
      const std::string& prefix,
      const std::vector<std::string>& expectedNexthops = {},
      bool expectDrop = false) {
    auto json = runCliJson({"show", "route"});
    XLOG(DBG2) << "show route JSON: " << folly::toPrettyJson(json);

    // JSON format: { "hostname": { "routeEntries": [...] } }
    for (const auto& [host, hostData] : json.items()) {
      if (!hostData.isObject() || !hostData.count("routeEntries")) {
        continue;
      }
      for (const auto& entry : hostData["routeEntries"]) {
        // networkAddress may include additional info like " (UCMP Active)"
        std::string networkAddr = entry["networkAddress"].asString();
        if (networkAddr.find(prefix) == 0) {
          // Found the route, now verify nexthops if requested
          const auto& nextHops = entry["nextHops"];

          if (expectDrop) {
            // Drop route should have no nexthops
            if (nextHops.empty()) {
              XLOG(INFO) << "  Found drop route for " << prefix;
              return true;
            } else {
              XLOG(WARNING)
                  << "  Route " << prefix << " has nexthops but expected drop";
              return false;
            }
          }

          if (!expectedNexthops.empty()) {
            // Verify expected nexthops are present
            std::set<std::string> foundNexthops;
            for (const auto& nh : nextHops) {
              foundNexthops.insert(nh["addr"].asString());
            }
            for (const auto& expectedNh : expectedNexthops) {
              if (foundNexthops.find(expectedNh) == foundNexthops.end()) {
                XLOG(WARNING) << "  Expected nexthop " << expectedNh
                              << " not found for route " << prefix;
                return false;
              }
            }
            XLOG(INFO) << "  Found route " << prefix << " with "
                       << nextHops.size() << " nexthop(s)";
          }
          return true;
        }
      }
    }
    return false;
  }

  /**
   * Add a null0 (blackhole) route.
   */
  void addDropRoute(const std::string& prefix, bool isIpv6 = false) {
    std::vector<std::string> args;
    if (isIpv6) {
      args = {"config", "protocol", "static", "ipv6", "route", prefix, "null0"};
    } else {
      args = {"config", "protocol", "static", "ip", "route", prefix, "null0"};
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to add drop route: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv4Route) {
  // Step 1: Find a valid interface to use
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Test setup:
  // 1. Add IP 10.99.1.1/24 to interface to create connected route
  // for 10.99.1.0/24
  // 2. Use 10.99.1.2 as nexthop for our test route (it's in the connected
  // subnet)
  const std::string interfaceIp = "10.99.1.1/24";
  const std::string testPrefix = "198.51.100.0/24"; // TEST-NET-2 from RFC 5737
  const std::string testNexthop = "10.99.1.2"; // In the connected subnet

  XLOG(INFO) << "[Step 2] Adding interface IP " << interfaceIp << " to "
             << interface.name;
  addInterfaceIp(interface.name, interfaceIp);

  XLOG(INFO) << "[Step 3] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 4] Adding route " << testPrefix << " via "
             << testNexthop;
  addRoute(testPrefix, testNexthop);

  XLOG(INFO) << "[Step 5] Verifying route was added with correct nexthop...";
  EXPECT_TRUE(routeExists(testPrefix, {testNexthop}))
      << "Route " << testPrefix << " not found or nexthop mismatch";
  XLOG(INFO) << "  Route " << testPrefix << " verified with nexthop";

  XLOG(INFO) << "[Step 6] Deleting route " << testPrefix;
  deleteRoute(testPrefix);

  XLOG(INFO) << "[Step 7] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  Route deleted successfully";

  XLOG(INFO) << "[Cleanup] Removing interface IP";
  deleteInterfaceIp(interface.name, interfaceIp);

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv6Route) {
  // Step 1: Find a valid interface to use
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Test setup:
  // 1. Add IPv6 2001:db8:99::1/64 to interface to create connected route
  // 2. Use 2001:db8:99::2 as nexthop for our test route (it's in the connected
  // subnet)
  const std::string interfaceIp = "2001:db8:99::1/64";
  const std::string testPrefix =
      "2001:db8:abcd::/48"; // Documentation range (RFC 3849)
  const std::string testNexthop = "2001:db8:99::2"; // In the connected subnet

  XLOG(INFO) << "[Step 2] Adding interface IPv6 " << interfaceIp << " to "
             << interface.name;
  addInterfaceIp(interface.name, interfaceIp, true);

  XLOG(INFO) << "[Step 3] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix, true);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 4] Adding IPv6 route " << testPrefix << " via "
             << testNexthop;
  addRoute(testPrefix, testNexthop, true);

  XLOG(INFO) << "[Step 5] Verifying route was added with correct nexthop...";
  EXPECT_TRUE(routeExists(testPrefix, {testNexthop}))
      << "Route " << testPrefix << " not found or nexthop mismatch";
  XLOG(INFO) << "  Route " << testPrefix << " verified with nexthop";

  XLOG(INFO) << "[Step 6] Deleting route " << testPrefix;
  deleteRoute(testPrefix, true);

  XLOG(INFO) << "[Step 7] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  Route deleted successfully";

  XLOG(INFO) << "[Cleanup] Removing interface IPv6";
  deleteInterfaceIp(interface.name, interfaceIp, true);

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv4DropRoute) {
  // Use a test prefix from TEST-NET-3 (RFC 5737)
  const std::string testPrefix = "203.0.113.0/24";

  XLOG(INFO) << "[Step 1] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 2] Adding drop route for " << testPrefix;
  addDropRoute(testPrefix);

  XLOG(INFO) << "[Step 3] Verifying drop route was added...";
  EXPECT_TRUE(routeExists(testPrefix, {}, true /* expectDrop */))
      << "Drop route " << testPrefix << " not found or not a drop route";
  XLOG(INFO) << "  Drop route " << testPrefix << " verified";

  XLOG(INFO) << "[Step 4] Deleting drop route " << testPrefix;
  deleteRoute(testPrefix);

  XLOG(INFO) << "[Step 5] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  Drop route deleted successfully";

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv6DropRoute) {
  // Use a test prefix from documentation range (RFC 3849)
  const std::string testPrefix = "2001:db8:dead::/48";

  XLOG(INFO) << "[Step 1] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix, true);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 2] Adding IPv6 drop route for " << testPrefix;
  addDropRoute(testPrefix, true);

  XLOG(INFO) << "[Step 3] Verifying drop route was added...";
  EXPECT_TRUE(routeExists(testPrefix, {}, true /* expectDrop */))
      << "Drop route " << testPrefix << " not found or not a drop route";
  XLOG(INFO) << "  Drop route " << testPrefix << " verified";

  XLOG(INFO) << "[Step 4] Deleting drop route " << testPrefix;
  deleteRoute(testPrefix, true);

  XLOG(INFO) << "[Step 5] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  Drop route deleted successfully";

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv4CpuRoute) {
  // Use a test prefix from TEST-NET-1 (RFC 5737)
  const std::string testPrefix = "192.0.2.0/24";

  XLOG(INFO) << "[Step 1] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 2] Adding CPU route for " << testPrefix;
  std::vector<std::string> args = {
      "config", "protocol", "static", "ip", "route", testPrefix, "cpu"};
  auto result = runCli(args);
  ASSERT_EQ(result.exitCode, 0) << "Failed to add CPU route: " << result.stderr;
  commitConfig();

  XLOG(INFO) << "[Step 3] Verifying CPU route was added...";
  EXPECT_TRUE(routeExists(testPrefix, {}, true /* expectDrop */))
      << "CPU route " << testPrefix << " not found";
  XLOG(INFO) << "  CPU route " << testPrefix << " verified";

  XLOG(INFO) << "[Step 4] Deleting CPU route " << testPrefix;
  deleteRoute(testPrefix);

  XLOG(INFO) << "[Step 5] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  CPU route deleted successfully";

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigIpRouteTest, AddAndDeleteIpv6CpuRoute) {
  // Use a test prefix from documentation range (RFC 3849)
  const std::string testPrefix = "2001:db8:cafe::/48";

  XLOG(INFO) << "[Step 1] Checking if test route already exists...";
  bool existedBefore = routeExists(testPrefix);
  if (existedBefore) {
    XLOG(INFO) << "  Route " << testPrefix
               << " already exists, cleaning up first";
    deleteRoute(testPrefix, true);
    ASSERT_FALSE(routeExists(testPrefix))
        << "Failed to clean up existing route";
  }

  XLOG(INFO) << "[Step 2] Adding IPv6 CPU route for " << testPrefix;
  std::vector<std::string> args = {
      "config", "protocol", "static", "ipv6", "route", testPrefix, "cpu"};
  auto result = runCli(args);
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to add IPv6 CPU route: " << result.stderr;
  commitConfig();

  XLOG(INFO) << "[Step 3] Verifying CPU route was added...";
  EXPECT_TRUE(routeExists(testPrefix, {}, true /* expectDrop */))
      << "CPU route " << testPrefix << " not found";
  XLOG(INFO) << "  CPU route " << testPrefix << " verified";

  XLOG(INFO) << "[Step 4] Deleting CPU route " << testPrefix;
  deleteRoute(testPrefix, true);

  XLOG(INFO) << "[Step 5] Verifying route was deleted...";
  EXPECT_FALSE(routeExists(testPrefix))
      << "Route " << testPrefix << " still exists after deletion";
  XLOG(INFO) << "  CPU route deleted successfully";

  XLOG(INFO) << "TEST PASSED";
}
