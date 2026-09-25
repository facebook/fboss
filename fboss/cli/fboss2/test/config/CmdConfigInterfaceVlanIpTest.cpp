// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Conv.h>
#include <folly/String.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// IP add/remove on VLAN SVIs, addressed as "vlan<id>". The name resolves to
// the interface whose vlanID matches, so SVIs are reachable even though their
// interface name is unset or auto-generated (fboss<id>). Seed:
//   2001 - SVI-backed VLAN: interface fboss2001 (vlanID 2001) with one v4 and
//          one v6 address
//   2003 - SVI whose interface has NO name: the motivating case, reachable
//          only through the vlanID
class CmdConfigInterfaceVlanIpTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceVlanIpTestFixture()
      : CmdConfigTestBase(
            "config_interface_vlan_ip_test_%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "vlans": [
      {"id": 2001, "name": "Vlan2001"},
      {"id": 2003, "name": "Vlan2003"}
    ],
    "interfaces": [
      {"intfID": 2001, "vlanID": 2001, "name": "fboss2001", "routerID": 0,
       "ipAddresses": ["10.0.1.1/24", "2001:db8:1::1/64"]},
      {"intfID": 2003, "vlanID": 2003, "routerID": 0, "ipAddresses": []}
    ]
  }
})") {}

 protected:
  cfg::SwitchConfig& swConfig() {
    return *ConfigSession::getInstance().getAgentConfig().sw();
  }

  cfg::Interface& interfaceById(int32_t intfID) {
    auto& intfs = *swConfig().interfaces();
    auto it = std::find_if(intfs.begin(), intfs.end(), [intfID](const auto& i) {
      return *i.intfID() == intfID;
    });
    if (it == intfs.end()) {
      throw std::runtime_error(
          folly::to<std::string>("test config is missing interface ", intfID));
    }
    return *it;
  }

  cfg::Interface& sviInterface() {
    return interfaceById(2001);
  }

  std::string configIp(const std::vector<std::string>& args) {
    setupTestableConfigSession("config interface", folly::join(" ", args));
    CmdConfigInterface cmd;
    return cmd.queryClient(localhost(), InterfacesConfig(args));
  }

  std::string deleteIp(const std::vector<std::string>& args) {
    setupTestableConfigSession("delete interface", folly::join(" ", args));
    CmdDeleteInterface cmd;
    return cmd.queryClient(localhost(), InterfaceDeleteConfig(args));
  }
};

TEST_F(CmdConfigInterfaceVlanIpTestFixture, addV4ToSvi) {
  auto result = configIp({"vlan2001", "ip-address", "10.0.2.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("10.0.2.1/24"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, addV6ToSvi) {
  auto result = configIp({"vlan2001", "ipv6-address", "2001:db8:2::1/64"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("2001:db8:2::1/64"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, deleteV4FromSvi) {
  auto result = deleteIp({"vlan2001", "ip-address", "10.0.1.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Not(Contains("10.0.1.1/24")));
  // The other family's address is untouched.
  EXPECT_THAT(*sviInterface().ipAddresses(), Contains("2001:db8:1::1/64"));
}

TEST_F(CmdConfigInterfaceVlanIpTestFixture, deleteV6FromSvi) {
  auto result = deleteIp({"vlan2001", "ipv6-address", "2001:db8:1::1/64"});
  EXPECT_THAT(result, HasSubstr("Successfully removed"));
  EXPECT_THAT(*sviInterface().ipAddresses(), Not(Contains("2001:db8:1::1/64")));
}

// A vlan<id> with no SVI fails to resolve and creates nothing.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, unknownVlanRejected) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfacesConfig({"vlan999", "ip-address", "10.0.0.1/24"}),
      std::invalid_argument);
  EXPECT_EQ(swConfig().vlans()->size(), 2);
  EXPECT_EQ(swConfig().interfaces()->size(), 2);
}

// The motivating case: an SVI with no interface name at all is still
// addressable as "vlan<id>", because resolution goes through vlanID.
TEST_F(CmdConfigInterfaceVlanIpTestFixture, unnamedSviResolvedByVlanName) {
  auto result = configIp({"vlan2003", "ip-address", "10.0.4.1/24"});
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(*interfaceById(2003).ipAddresses(), Contains("10.0.4.1/24"));
}

} // namespace facebook::fboss
