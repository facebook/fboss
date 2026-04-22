/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/commands/delete/interface/ipv6/nd/CmdDeleteInterfaceIpv6Nd.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class CmdDeleteInterfaceIpv6NdTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteInterfaceIpv6NdTestFixture()
      : CmdConfigTestBase(
            "fboss_delete_ipv6_nd_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [],
    "interfaces": [
      {
        "intfID": 1,
        "routerID": 0,
        "vlanID": 1,
        "name": "eth1/1/1",
        "mtu": 1500,
        "ndp": {
          "routerAdvertisementSeconds": 30,
          "curHopLimit": 64,
          "routerLifetime": 900,
          "prefixValidLifetimeSeconds": 86400,
          "prefixPreferredLifetimeSeconds": 43200,
          "routerAdvertisementManagedBit": true,
          "routerAdvertisementOtherBit": true,
          "routerAddress": "2001:db8::1"
        }
      },
      {
        "intfID": 2,
        "routerID": 0,
        "vlanID": 2,
        "name": "eth1/2/1",
        "mtu": 1500
      }
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";
};

// ---------------------------------------------------------------------------
// NdpDeleteAttrs parsing tests
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, ndpDeleteAttrsEmpty) {
  setupTestableConfigSession();
  EXPECT_THROW(NdpDeleteAttrs({}), std::invalid_argument);
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, ndpDeleteAttrsUnknown) {
  setupTestableConfigSession();
  try {
    NdpDeleteAttrs({"bogus"}); // NOLINT(bugprone-unused-raii)
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown nd attribute"));
    EXPECT_THAT(e.what(), HasSubstr("bogus"));
  }
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, ndpDeleteAttrsCaseInsensitive) {
  setupTestableConfigSession();
  NdpDeleteAttrs attrs({"RA-INTERVAL"});
  ASSERT_EQ(attrs.getAttributes().size(), 1);
  EXPECT_EQ(attrs.getAttributes()[0], "ra-interval");
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, ndpDeleteAttrsMultiple) {
  setupTestableConfigSession();
  NdpDeleteAttrs attrs({"ra-interval", "hop-limit"});
  ASSERT_EQ(attrs.getAttributes().size(), 2);
  EXPECT_EQ(attrs.getAttributes()[0], "ra-interval");
  EXPECT_EQ(attrs.getAttributes()[1], "hop-limit");
}

// ---------------------------------------------------------------------------
// queryClient: individual attribute reset tests
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientResetsRaInterval) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"ra-interval"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerAdvertisementSeconds(), 0);
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientResetsHopLimit) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd hop-limit");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"hop-limit"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->curHopLimit(), 255);
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientResetsRaLifetime) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-lifetime");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"ra-lifetime"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerLifetime(), 1800);
}

TEST_F(
    CmdDeleteInterfaceIpv6NdTestFixture,
    queryClientResetsPrefixValidLifetime) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd prefix-valid-lifetime");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"prefix-valid-lifetime"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->prefixValidLifetimeSeconds(), 2592000);
}

TEST_F(
    CmdDeleteInterfaceIpv6NdTestFixture,
    queryClientResetsPrefixPreferredLifetime) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd prefix-preferred-lifetime");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"prefix-preferred-lifetime"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->prefixPreferredLifetimeSeconds(), 604800);
}

TEST_F(
    CmdDeleteInterfaceIpv6NdTestFixture,
    queryClientResetsManagedConfigFlag) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd managed-config-flag");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"managed-config-flag"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_FALSE(*ifaces[0].ndp()->routerAdvertisementManagedBit());
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientResetsOtherConfigFlag) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd other-config-flag");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"other-config-flag"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_FALSE(*ifaces[0].ndp()->routerAdvertisementOtherBit());
}

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientResetsRaAddress) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-address");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"ra-address"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_FALSE(ifaces[0].ndp()->routerAddress().has_value());
}

// ---------------------------------------------------------------------------
// queryClient: idempotent — no NDP config on interface
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientIdempotentNoNdp) {
  // eth1/2/1 has no ndp block in the initial config
  setupTestableConfigSession(cmdPrefix_, "eth1/2/1 ipv6 nd ra-interval");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/2/1"});
  NdpDeleteAttrs attrs({"ra-interval"});

  // Should not throw — silently skip
  EXPECT_NO_THROW({
    auto result = cmd.queryClient(localhost(), interfaces, attrs);
    EXPECT_THAT(result, HasSubstr("Successfully reset"));
  });
}

// ---------------------------------------------------------------------------
// queryClient: multiple attributes at once
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, queryClientMultipleAttrs) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval hop-limit");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpDeleteAttrs attrs({"ra-interval", "hop-limit"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerAdvertisementSeconds(), 0);
  EXPECT_EQ(*ifaces[0].ndp()->curHopLimit(), 255);
}

// ---------------------------------------------------------------------------
// printOutput test
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceIpv6NdTestFixture, printOutput) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval");
  auto cmd = CmdDeleteInterfaceIpv6Nd();
  testing::internal::CaptureStdout();
  cmd.printOutput("test output");
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("test output"));
}

} // namespace facebook::fboss
