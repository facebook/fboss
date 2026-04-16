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

#include "fboss/cli/fboss2/commands/config/interface/ipv6/nd/CmdConfigInterfaceIpv6Nd.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class CmdConfigInterfaceIpv6NdTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceIpv6NdTestFixture()
      : CmdConfigTestBase(
            "fboss_ipv6_nd_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [],
    "interfaces": [
      {
        "intfID": 1,
        "routerID": 0,
        "vlanID": 1,
        "name": "eth1/1/1",
        "mtu": 1500
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

  void SetUp() override {
    CmdConfigTestBase::SetUp();
  }

 protected:
  const std::string cmdPrefix_ = "config interface";
};

// ---------------------------------------------------------------------------
// NdpConfigAttrs parsing tests
// ---------------------------------------------------------------------------

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsEmpty) {
  setupTestableConfigSession();
  EXPECT_THROW(NdpConfigAttrs({}), std::invalid_argument);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsUnknownAttr) {
  setupTestableConfigSession();
  try {
    NdpConfigAttrs({"bogus-attr", "value"}); // NOLINT(bugprone-unused-raii)
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown nd attribute"));
    EXPECT_THAT(e.what(), HasSubstr("bogus-attr"));
  }
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsMissingValue) {
  setupTestableConfigSession();
  try {
    NdpConfigAttrs({"ra-interval"}); // NOLINT(bugprone-unused-raii)
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for nd attribute"));
    EXPECT_THAT(e.what(), HasSubstr("ra-interval"));
  }
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsValueIsAnotherAttr) {
  setupTestableConfigSession();
  try {
    NdpConfigAttrs( // NOLINT(bugprone-unused-raii)
        {"ra-interval", "ra-lifetime"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for nd attribute"));
    EXPECT_THAT(e.what(), HasSubstr("ra-interval"));
    EXPECT_THAT(e.what(), HasSubstr("ra-lifetime"));
  }
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsParseValueful) {
  setupTestableConfigSession();
  NdpConfigAttrs attrs({"ra-interval", "30"});
  ASSERT_EQ(attrs.getAttributes().size(), 1);
  EXPECT_EQ(attrs.getAttributes()[0].first, "ra-interval");
  EXPECT_EQ(attrs.getAttributes()[0].second, "30");
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsParseValueless) {
  setupTestableConfigSession();
  NdpConfigAttrs attrs({"managed-config-flag"});
  ASSERT_EQ(attrs.getAttributes().size(), 1);
  EXPECT_EQ(attrs.getAttributes()[0].first, "managed-config-flag");
  EXPECT_EQ(attrs.getAttributes()[0].second, "");
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsCaseInsensitive) {
  setupTestableConfigSession();
  NdpConfigAttrs attrs({"RA-INTERVAL", "30"});
  ASSERT_EQ(attrs.getAttributes().size(), 1);
  EXPECT_EQ(attrs.getAttributes()[0].first, "ra-interval");
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, ndpAttrsMultiple) {
  setupTestableConfigSession();
  NdpConfigAttrs attrs({"ra-interval", "30", "hop-limit", "64"});
  ASSERT_EQ(attrs.getAttributes().size(), 2);
  EXPECT_EQ(attrs.getAttributes()[0].first, "ra-interval");
  EXPECT_EQ(attrs.getAttributes()[0].second, "30");
  EXPECT_EQ(attrs.getAttributes()[1].first, "hop-limit");
  EXPECT_EQ(attrs.getAttributes()[1].second, "64");
}

// ---------------------------------------------------------------------------
// queryClient: individual attribute tests
// ---------------------------------------------------------------------------

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsRaInterval) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval 30");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"ra-interval", "30"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("ra-interval=30"));

  auto& session = ConfigSession::getInstance();
  auto& ifaces = *session.getAgentConfig().sw()->interfaces();
  EXPECT_TRUE(ifaces[0].ndp().has_value());
  EXPECT_EQ(*ifaces[0].ndp()->routerAdvertisementSeconds(), 30);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsRaLifetime) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-lifetime 1800");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"ra-lifetime", "1800"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("ra-lifetime=1800"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerLifetime(), 1800);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsHopLimit) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd hop-limit 64");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"hop-limit", "64"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("hop-limit=64"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->curHopLimit(), 64);
}

TEST_F(
    CmdConfigInterfaceIpv6NdTestFixture,
    queryClientSetsPrefixValidLifetime) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd prefix-valid-lifetime 2592000");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"prefix-valid-lifetime", "2592000"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("prefix-valid-lifetime=2592000"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->prefixValidLifetimeSeconds(), 2592000);
}

TEST_F(
    CmdConfigInterfaceIpv6NdTestFixture,
    queryClientSetsPrefixPreferredLifetime) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd prefix-preferred-lifetime 604800");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"prefix-preferred-lifetime", "604800"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("prefix-preferred-lifetime=604800"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->prefixPreferredLifetimeSeconds(), 604800);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsManagedConfigFlag) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd managed-config-flag");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"managed-config-flag"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("managed-config-flag=true"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_TRUE(*ifaces[0].ndp()->routerAdvertisementManagedBit());
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsOtherConfigFlag) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd other-config-flag");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"other-config-flag"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("other-config-flag=true"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_TRUE(*ifaces[0].ndp()->routerAdvertisementOtherBit());
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientSetsRaAddress) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 ipv6 nd ra-address 2001:db8::1");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"ra-address", "2001:db8::1"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("ra-address=2001:db8::1"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerAddress(), "2001:db8::1");
}

// ---------------------------------------------------------------------------
// queryClient: validation error tests
// ---------------------------------------------------------------------------

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientRaIntervalNotInteger) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval abc");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"ra-interval", "abc"});

  EXPECT_THROW(
      cmd.queryClient(localhost(), interfaces, attrs), std::invalid_argument);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientRaIntervalNegative) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd ra-interval -1");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"ra-interval", "-1"});

  try {
    cmd.queryClient(localhost(), interfaces, attrs);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("out of range"));
  }
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientHopLimitOutOfRange) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd hop-limit 256");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"hop-limit", "256"});

  try {
    cmd.queryClient(localhost(), interfaces, attrs);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("0-255"));
  }
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientHopLimitZeroIsValid) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ipv6 nd hop-limit 0");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs({"hop-limit", "0"});

  EXPECT_NO_THROW(cmd.queryClient(localhost(), interfaces, attrs));
}

// ---------------------------------------------------------------------------
// queryClient: multi-port and combined attrs
// ---------------------------------------------------------------------------

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientMultiPort) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 eth1/2/1 ipv6 nd ra-interval 60");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});
  NdpConfigAttrs attrs({"ra-interval", "60"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));
  EXPECT_THAT(result, HasSubstr("ra-interval=60"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerAdvertisementSeconds(), 60);
  EXPECT_EQ(*ifaces[1].ndp()->routerAdvertisementSeconds(), 60);
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, queryClientMultipleAttrs) {
  setupTestableConfigSession(
      cmdPrefix_,
      "eth1/1/1 ipv6 nd ra-interval 30 hop-limit 64 managed-config-flag");
  auto cmd = CmdConfigInterfaceIpv6Nd();
  utils::InterfaceList interfaces({"eth1/1/1"});
  NdpConfigAttrs attrs(
      {"ra-interval", "30", "hop-limit", "64", "managed-config-flag"});

  auto result = cmd.queryClient(localhost(), interfaces, attrs);

  EXPECT_THAT(result, HasSubstr("ra-interval=30"));
  EXPECT_THAT(result, HasSubstr("hop-limit=64"));
  EXPECT_THAT(result, HasSubstr("managed-config-flag=true"));

  auto& ifaces =
      *ConfigSession::getInstance().getAgentConfig().sw()->interfaces();
  EXPECT_EQ(*ifaces[0].ndp()->routerAdvertisementSeconds(), 30);
  EXPECT_EQ(*ifaces[0].ndp()->curHopLimit(), 64);
  EXPECT_TRUE(*ifaces[0].ndp()->routerAdvertisementManagedBit());
}

TEST_F(CmdConfigInterfaceIpv6NdTestFixture, printOutput) {
  auto cmd = CmdConfigInterfaceIpv6Nd();
  std::string msg =
      "Successfully configured interface(s) eth1/1/1: ra-interval=30";

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  cmd.printOutput(msg);
  std::cout.rdbuf(old);

  EXPECT_EQ(buffer.str(), msg + "\n");
}

} // namespace facebook::fboss
