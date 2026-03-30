// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdDeleteConfigInterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteConfigInterfaceTestFixture()
      : CmdConfigTestBase(
            "delete_interface_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "interfaces": [
      {
        "intfID": 1,
        "name": "eth1/1/1",
        "vlanID": 100,
        "routerID": 0,
        "ipAddresses": []
      }
    ]
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";
};

// ============================================================================
// InterfaceDeleteConfig Validation Tests
// ============================================================================

// Test valid delete config with single interface
TEST_F(CmdDeleteConfigInterfaceTestFixture, validSingleInterface) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  EXPECT_EQ(config.getInterfaces()[0].name(), "eth1/1/1");
  EXPECT_EQ(config.getAttributes()[0].first, "ip-address");
  EXPECT_EQ(config.getAttributes()[0].second, "10.0.0.1/24");
}

// Test IPv6 address
TEST_F(CmdDeleteConfigInterfaceTestFixture, validIpv6Address) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "ipv6-address", "2001:db8::1/64"});
  EXPECT_EQ(config.getInterfaces()[0].name(), "eth1/1/1");
  EXPECT_EQ(config.getAttributes()[0].first, "ipv6-address");
  EXPECT_EQ(config.getAttributes()[0].second, "2001:db8::1/64");
}

// Test case-insensitive attribute names
TEST_F(CmdDeleteConfigInterfaceTestFixture, caseInsensitiveAttribute) {
  setupTestableConfigSession();
  InterfaceDeleteConfig config({"eth1/1/1", "IP-ADDRESS", "10.0.0.1/24"});
  // Attribute should be normalized to lowercase
  EXPECT_EQ(config.getAttributes()[0].first, "ip-address");
}

// Test unknown interface throws (eth1/2/1 is not in the test config)
TEST_F(CmdDeleteConfigInterfaceTestFixture, unknownInterfaceThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig(
          {"eth1/1/1", "eth1/2/1", "ip-address", "10.0.0.1/24"}),
      std::invalid_argument);
}

// Test too few arguments throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, tooFewArgumentsThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "ip-address"}), std::invalid_argument);
}

// Test unknown attribute throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, unknownAttributeThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "description", "test"}),
      std::invalid_argument);
}

// Test invalid IP address format throws
TEST_F(CmdDeleteConfigInterfaceTestFixture, invalidIpAddressThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "ip-address", "invalid"}),
      std::exception);
}

// Test IP version mismatch throws during construction
TEST_F(CmdDeleteConfigInterfaceTestFixture, ipVersionMismatchThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      (InterfaceDeleteConfig({"eth1/1/1", "ip-address", "2001:db8::1/64"})),
      std::invalid_argument);
}

// ============================================================================
// CmdDeleteConfigInterface::queryClient Tests
// ============================================================================

// Test deleting existing IPv4 address
TEST_F(CmdDeleteConfigInterfaceTestFixture, deleteExistingIpv4Address) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ip-address 10.0.0.1/24");

  // First add the IP address
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& intfs = *config.sw()->interfaces();
  for (auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      intf.ipAddresses()->push_back("10.0.0.1/24");
      break;
    }
  }
  session.saveConfig();

  // Now delete it
  auto cmd = CmdDeleteInterface();
  InterfaceDeleteConfig deleteConfig({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  auto result = cmd.queryClient(localhost(), deleteConfig);

  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(result, HasSubstr("10.0.0.1/24"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  // Verify it was removed
  auto& updatedIntfs = *config.sw()->interfaces();
  for (const auto& intf : updatedIntfs) {
    if (*intf.name() == "eth1/1/1") {
      auto& ips = *intf.ipAddresses();
      EXPECT_EQ(std::find(ips.begin(), ips.end(), "10.0.0.1/24"), ips.end());
    }
  }
}

// Test deleting non-existent IP address (idempotent)
TEST_F(CmdDeleteConfigInterfaceTestFixture, deleteNonExistentIpAddress) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 ip-address 10.0.0.1/24");

  auto cmd = CmdDeleteInterface();
  InterfaceDeleteConfig deleteConfig({"eth1/1/1", "ip-address", "10.0.0.1/24"});
  auto result = cmd.queryClient(localhost(), deleteConfig);

  EXPECT_THAT(result, HasSubstr("not configured"));
}

} // namespace facebook::fboss
