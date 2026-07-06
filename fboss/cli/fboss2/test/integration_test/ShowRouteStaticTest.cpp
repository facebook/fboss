// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2 show route static [ip | ipv6]'.
 *
 * Rather than driving 'config protocol static ... route' + 'config session
 * commit' (the runtime config-apply path), this test injects static routes
 * directly into the agent config file and restarts the agent. This exercises
 * the same startup config-application path the agent uses on boot and avoids
 * the runtime config-reload code path.
 *
 * Flow:
 * 1. Read the agent config, save the original for restore.
 * 2. Inject a connected subnet (so nexthops resolve) plus static routes
 *    (nexthop, null0, cpu, IPv6 nexthop) into sw.staticRoutes* and restart.
 * 3. Verify they appear under 'show route static' / '... ip' / '... ipv6' with
 *    the expected prefix and nexthop, that families are filtered correctly,
 *    and that the connected (non-static) route does NOT appear (client filter).
 * 4. TearDown restores the original config and restarts the agent.
 *
 * NOTE: 'show route static mpls' is not covered here (no config knob installs a
 * static MPLS route); that path is covered by the unit test.
 *
 * Requirements:
 * - FBOSS agent must be running, managed by systemd (fboss_sw_agent).
 * - Test must run as root (writes the agent config, restarts the service).
 */

#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
constexpr auto kAgentConfigPath = "/etc/coop/agent.conf";

// runCmd() execs the literal binary (folly::Subprocess does not search PATH),
// so a bare "systemctl" fails. Resolve its absolute location from PATH so we
// don't hardcode a distro-specific path (/usr/bin vs /bin). Returns "" if not
// found.
std::string resolveFromPath(const std::string& name) {
  // NOLINTNEXTLINE(concurrency-mt-unsafe) - test setup is single-threaded.
  const char* path = std::getenv("PATH");
  if (path == nullptr) {
    return "";
  }
  std::stringstream ss(path);
  std::string dir;
  while (std::getline(ss, dir, ':')) {
    if (dir.empty()) {
      continue;
    }
    std::string full = dir;
    full += '/';
    full += name;
    if (::access(full.c_str(), X_OK) == 0) {
      return full;
    }
  }
  return "";
}

// Test prefixes from RFC 5737 / RFC 3849 documentation ranges.
constexpr auto kV4Nexthop = "198.51.100.0/24";
constexpr auto kV4Null0 = "203.0.113.0/24";
constexpr auto kV4Cpu = "192.0.2.0/24";
constexpr auto kV6Nexthop = "2001:db8:abcd::/48";
constexpr auto kV4ConnectedSubnet = "10.99.1.0/24";
constexpr auto kV4InterfaceIp = "10.99.1.1/24";
constexpr auto kV6InterfaceIp = "2001:db8:99::1/64";
constexpr auto kV4NexthopAddr = "10.99.1.2";
constexpr auto kV6NexthopAddr = "2001:db8:99::2";
} // namespace

class ShowRouteStaticTest : public Fboss2IntegrationTest {
 protected:
  std::string savedConfig_;
  bool configModified_ = false;

  void TearDown() override {
    if (configModified_) {
      writeFile(kAgentConfigPath, savedConfig_);
      auto systemctl = resolveFromPath("systemctl");
      if (!systemctl.empty()) {
        runCmd({systemctl, "restart", "fboss_sw_agent"});
        waitForAgentReady();
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  std::string readFile(const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }

  void writeFile(const std::string& path, const std::string& content) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
  }

  static folly::dynamic withNexthops(
      const std::string& prefix,
      const std::vector<std::string>& nexthops) {
    folly::dynamic route = folly::dynamic::object;
    route["prefix"] = prefix;
    folly::dynamic nhs = folly::dynamic::array;
    for (const auto& nh : nexthops) {
      nhs.push_back(nh);
    }
    route["nexthops"] = nhs;
    route["routerID"] = 0;
    return route;
  }

  static folly::dynamic noNexthops(const std::string& prefix) {
    folly::dynamic route = folly::dynamic::object;
    route["prefix"] = prefix;
    route["routerID"] = 0;
    return route;
  }

  static void ensureArray(folly::dynamic& obj, const std::string& key) {
    if (!obj.count(key) || !obj[key].isArray()) {
      obj[key] = folly::dynamic::array;
    }
  }

  // Injects a connected subnet + static routes into the agent config and
  // restarts the agent. Returns false (and the caller should SKIP) if the
  // config has no routerID-0 interface to attach the subnet to.
  bool injectStaticRoutesAndRestart() {
    savedConfig_ = readFile(kAgentConfigPath);
    if (savedConfig_.empty()) {
      return false;
    }
    folly::dynamic cfg = folly::parseJson(savedConfig_);
    if (!cfg.isObject() || !cfg.count("sw")) {
      return false;
    }
    auto& sw = cfg["sw"];

    // Attach the connected subnets to the first routerID-0 interface so the
    // static-route nexthops resolve.
    bool attached = false;
    if (sw.count("interfaces") && sw["interfaces"].isArray()) {
      for (auto& intf : sw["interfaces"]) {
        if (intf.getDefault("routerID", 0).asInt() == 0) {
          ensureArray(intf, "ipAddresses");
          intf["ipAddresses"].push_back(kV4InterfaceIp);
          intf["ipAddresses"].push_back(kV6InterfaceIp);
          attached = true;
          break;
        }
      }
    }
    if (!attached) {
      return false;
    }

    ensureArray(sw, "staticRoutesWithNhops");
    ensureArray(sw, "staticRoutesToNull");
    ensureArray(sw, "staticRoutesToCPU");
    sw["staticRoutesWithNhops"].push_back(
        withNexthops(kV4Nexthop, {kV4NexthopAddr}));
    sw["staticRoutesWithNhops"].push_back(
        withNexthops(kV6Nexthop, {kV6NexthopAddr}));
    sw["staticRoutesToNull"].push_back(noNexthops(kV4Null0));
    sw["staticRoutesToCPU"].push_back(noNexthops(kV4Cpu));

    auto systemctl = resolveFromPath("systemctl");
    if (systemctl.empty()) {
      return false;
    }

    // Mark modified before writing so TearDown restores even if restart fails.
    configModified_ = true;
    writeFile(kAgentConfigPath, folly::toJson(cfg));

    auto r = runCmd({systemctl, "restart", "fboss_sw_agent"});
    if (r.exitCode != 0) {
      return false;
    }
    waitForAgentReady();
    return true;
  }

  /**
   * Look for prefix in the output of the given 'show route static ...' command.
   * If expectedNexthop is non-empty, require a nexthop with that addr (an IP,
   * "null0" or "cpu").
   */
  bool staticRouteExists(
      const std::vector<std::string>& showCmd,
      const std::string& prefix,
      const std::string& expectedNexthop = "") {
    auto json = runCliJson(showCmd);
    XLOG(DBG2) << "show output: " << folly::toPrettyJson(json);
    for (const auto& [host, hostData] : json.items()) {
      if (!hostData.isObject() || !hostData.count("routeEntries")) {
        continue;
      }
      for (const auto& entry : hostData["routeEntries"]) {
        // networkAddress is "<prefix>" possibly with trailing annotations;
        // match the prefix token exactly up to any whitespace.
        std::string networkAddr = entry["networkAddress"].asString();
        std::string token = networkAddr.substr(0, networkAddr.find(' '));
        if (token != prefix) {
          continue;
        }
        if (expectedNexthop.empty()) {
          return true;
        }
        for (const auto& nh : entry["nextHops"]) {
          if (nh["addr"].asString() == expectedNexthop) {
            return true;
          }
        }
        return false;
      }
    }
    return false;
  }
};

TEST_F(ShowRouteStaticTest, ShowStaticRoutes) {
  if (!injectStaticRoutesAndRestart()) {
    GTEST_SKIP() << "Could not inject static routes into agent config "
                    "(missing config, no routerID-0 interface, or restart "
                    "failed)";
  }

  // IPv4 view: nexthop, null0, cpu present with the right nexthop; IPv6 absent.
  EXPECT_TRUE(staticRouteExists(
      {"show", "route", "static", "ip"}, kV4Nexthop, kV4NexthopAddr));
  EXPECT_TRUE(
      staticRouteExists({"show", "route", "static", "ip"}, kV4Null0, "null0"));
  EXPECT_TRUE(
      staticRouteExists({"show", "route", "static", "ip"}, kV4Cpu, "cpu"));
  EXPECT_FALSE(
      staticRouteExists({"show", "route", "static", "ip"}, kV6Nexthop));

  // IPv6 view: the v6 nexthop route present; v4 routes absent.
  EXPECT_TRUE(staticRouteExists(
      {"show", "route", "static", "ipv6"}, kV6Nexthop, kV6NexthopAddr));
  EXPECT_FALSE(
      staticRouteExists({"show", "route", "static", "ipv6"}, kV4Nexthop));

  // Unified view contains all four static routes.
  EXPECT_TRUE(staticRouteExists({"show", "route", "static"}, kV4Nexthop));
  EXPECT_TRUE(staticRouteExists({"show", "route", "static"}, kV4Null0));
  EXPECT_TRUE(staticRouteExists({"show", "route", "static"}, kV4Cpu));
  EXPECT_TRUE(staticRouteExists({"show", "route", "static"}, kV6Nexthop));

  // The connected route is NOT a static route -> must be filtered out.
  EXPECT_FALSE(
      staticRouteExists({"show", "route", "static", "ip"}, kV4ConnectedSubnet))
      << "Connected route leaked into 'show route static'";
}
